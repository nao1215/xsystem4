/* Copyright (C) 2022 kichikuou <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <cglm/cglm.h>

#include "system4.h"
#include "system4/aar.h"
#include "system4/cg.h"
#include "system4/hashtable.h"

#include "3d_internal.h"
#include "reign.h"

#define NR_WEIGHTS 4

struct vertex {
	GLfloat pos[3];
	GLfloat normal[3];
	GLfloat uv[2];
	GLfloat tangent[4];
	GLint bone_id[NR_WEIGHTS];
	GLfloat bone_weight[NR_WEIGHTS];
};

static struct archive_data *get_aar_entry(struct archive *aar, const char *dir, const char *name, const char *ext)
{
	char *path = xmalloc(strlen(dir) + strlen(name) + strlen(ext) + 2);
	sprintf(path, "%s\\%s%s", dir, name, ext);
	struct archive_data *dfile = archive_get_by_name(aar, path);
	free(path);
	return dfile;
}

static GLuint load_texture(struct archive *aar, const char *path, const char *name)
{
	struct archive_data *dfile = get_aar_entry(aar, path, name, "");
	if (!dfile) {
		WARNING("cannot load texture %s\\%s", path, name);
		return 0;
	}
	struct cg *cg = cg_load_data(dfile);
	if (!cg) {
		WARNING("cg_load_data failed: %s", dfile->name);
		archive_free_data(dfile);
		return 0;
	}
	archive_free_data(dfile);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cg->metrics.w, cg->metrics.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, cg->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	cg_free(cg);
	return texture;
}

static bool init_material(struct material *material, const struct pol_material *m, struct amt *amt, struct archive *aar, const char *path)
{
	if (!m->textures[COLOR_MAP]) {
		WARNING("No color texture");
		return false;
	}
	material->color_map = load_texture(aar, path, m->textures[COLOR_MAP]);
	if (!material->color_map)
		return false;

	if (m->textures[SPECULAR_MAP]) {
		material->specular_map = load_texture(aar, path, m->textures[SPECULAR_MAP]);
	}
	if (m->textures[NORMAL_MAP]) {
		material->normal_map = load_texture(aar, path, m->textures[NORMAL_MAP]);
	}

	struct amt_material *amt_m = amt ? amt_find_material(amt, m->name) : NULL;
	if (amt_m) {
		material->specular_strength = amt_m->fields[AMT_SPECULAR_STRENGTH];
		material->specular_shininess = amt_m->fields[AMT_SPECULAR_SHININESS];
		if (amt->version >= 5) {
			material->rim_exponent = amt_m->fields[AMT_RIM_EXPONENT];
			material->rim_color[0] = amt_m->fields[AMT_RIM_R];
			material->rim_color[1] = amt_m->fields[AMT_RIM_G];
			material->rim_color[2] = amt_m->fields[AMT_RIM_B];
		}
	}

	return true;
}

static void destroy_material(struct material *material)
{
	if (material->color_map)
		glDeleteTextures(1, &material->color_map);
	if (material->specular_map)
		glDeleteTextures(1, &material->specular_map);
	if (material->normal_map)
		glDeleteTextures(1, &material->normal_map);
}

static int cmp_by_bone_weight(const void *lhs, const void *rhs)
{
	float l = ((struct pol_bone_weight *)lhs)->weight;
	float r = ((struct pol_bone_weight *)rhs)->weight;
	return (l < r) - (l > r);  // descending order.
}

static void sort_and_normalize_bone_weights(struct pol_vertex *v)
{
	qsort(v->weights, v->nr_weights, sizeof(struct pol_bone_weight), cmp_by_bone_weight);
	float total = 0.0;
	for (uint32_t i = 0; i < v->nr_weights && i < NR_WEIGHTS; i++) {
		total += v->weights[i].weight;
	}
	for (uint32_t i = 0; i < v->nr_weights && i < NR_WEIGHTS; i++) {
		v->weights[i].weight /= total;
	}
}

static void calc_tangent(struct pol_mesh *m, struct pol_triangle *t, vec4 tangent[3])
{
	vec3 v1, v2;
	glm_vec3_sub(m->vertices[t->vert_index[1]].pos, m->vertices[t->vert_index[0]].pos, v1);
	glm_vec3_sub(m->vertices[t->vert_index[2]].pos, m->vertices[t->vert_index[0]].pos, v2);

	vec2 w1, w2;
	glm_vec2_sub(m->uvs[t->uv_index[1]], m->uvs[t->uv_index[0]], w1);
	glm_vec2_sub(m->uvs[t->uv_index[2]], m->uvs[t->uv_index[0]], w2);

	float r = 1.0 / (w1[0] * w2[1] - w2[0] * w1[1]);
	if (!isfinite(r))  // degenerate uv triangle
		r = 1.0;  // ??

	vec3 sdir, tdir;
	glm_vec3_scale(v1, w2[1], sdir);
	glm_vec3_muladds(v2, -w1[1], sdir);
	glm_vec3_scale(sdir, r, sdir);

	glm_vec3_scale(v2, w1[0], tdir);
	glm_vec3_muladds(v1, -w2[0], tdir);
	glm_vec3_scale(tdir, r, tdir);

	for (int i = 0; i < 3; i++) {
		vec3 s;
		glm_vec3_copy(sdir, s);

		// Gram-Schmidt orthogonalize.
		glm_vec3_muladds(t->normals[i], -glm_vec3_dot(t->normals[i], s), s);
		glm_vec3_normalize(s);

		// Calculate handedness.
		vec3 c;
		glm_vec3_cross(t->normals[i], sdir, c);
		float w = (glm_vec3_dot(c, tdir) < 0.0) ? -1.0 : 1.0;

		glm_vec4(s, w, tangent[i]);
	}
}

static void add_mesh(struct model *model, struct pol_mesh *m, int material_index, int material, struct RE_renderer *r)
{
	bool has_normal_map = model->materials[material].normal_map != 0;

	struct vertex *buf = xmalloc(m->nr_triangles * 3 * sizeof(struct vertex));
	int v = 0;
	for (uint32_t i = 0; i < m->nr_triangles; i++) {
		struct pol_triangle *t = &m->triangles[i];
		if (material_index >= 0 && t->material != (uint32_t)material_index)
			continue;
		vec4 tangent[3];
		if (has_normal_map)
			calc_tangent(m, t, tangent);
		for (int j = 0; j < 3; j++) {
			struct pol_vertex *vert = &m->vertices[t->vert_index[j]];
			glm_vec3_copy(vert->pos, buf[v].pos);
			glm_vec3_copy(t->normals[j], buf[v].normal);
			glm_vec2_copy(m->uvs[t->uv_index[j]], buf[v].uv);
			glm_vec4_copy(tangent[j], buf[v].tangent);
			sort_and_normalize_bone_weights(vert);
			for (uint32_t k = 0; k < NR_WEIGHTS; k++) {
				if (model->bone_map && k < vert->nr_weights) {
					struct bone *bone = ht_get_int(model->bone_map, vert->weights[k].bone, NULL);
					if (!bone)
						WARNING("%s: invalid bone id in vertex data", model->path);
					buf[v].bone_id[k] = bone ? bone->index : -1;
					buf[v].bone_weight[k] = vert->weights[k].weight;
				} else {
					buf[v].bone_id[k] = -1;
					buf[v].bone_weight[k] = 0.0;
				}
			}
			v++;
		}
	}
	if (v == 0) {
		free(buf);
		return;
	}
	model->meshes = xrealloc_array(model->meshes, model->nr_meshes, model->nr_meshes + 1, sizeof(struct mesh));
	struct mesh *mesh = &model->meshes[model->nr_meshes++];
	mesh->material = material;
	mesh->nr_vertices = v;

	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);
	glGenBuffers(1, &mesh->attr_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->attr_buffer);

	glEnableVertexAttribArray(r->shader.vertex);
	glVertexAttribPointer(r->shader.vertex, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (const void *)0);
	glEnableVertexAttribArray(r->vertex_normal);
	glVertexAttribPointer(r->vertex_normal, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (const void *)offsetof(struct vertex, normal));
	glEnableVertexAttribArray(r->vertex_uv);
	glVertexAttribPointer(r->vertex_uv, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (const void *)offsetof(struct vertex, uv));
	glEnableVertexAttribArray(r->vertex_tangent);
	glVertexAttribPointer(r->vertex_tangent, 4, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (const void *)offsetof(struct vertex, tangent));
	glEnableVertexAttribArray(r->vertex_bone_index);
	glVertexAttribIPointer(r->vertex_bone_index, NR_WEIGHTS, GL_INT, sizeof(struct vertex), (const void *)offsetof(struct vertex, bone_id));
	glEnableVertexAttribArray(r->vertex_bone_weight);
	glVertexAttribPointer(r->vertex_bone_weight, NR_WEIGHTS, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (const void *)offsetof(struct vertex, bone_weight));
	glBufferData(GL_ARRAY_BUFFER, mesh->nr_vertices * sizeof(struct vertex), buf, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	free(buf);
}

static void destroy_mesh(struct mesh *mesh)
{
	glDeleteVertexArrays(1, &mesh->vao);
	glDeleteBuffers(1, &mesh->attr_buffer);
}

static struct bone *add_bone(struct model *model, struct pol *pol, struct pol_bone *pol_bone)
{
	struct bone *bone = ht_get_int(model->bone_map, pol_bone->id, NULL);
	if (bone)
		return bone;  // already added.

	struct bone *parent = NULL;
	if (pol_bone->parent >= 0) {
		// Parent bone must appear before its children in model->bones.
		struct pol_bone *pol_parent = pol_find_bone(pol, pol_bone->parent);
		if (!pol_parent)
			ERROR("Parent bone of \"%s\" is not found", pol_bone->name);
		parent = add_bone(model, pol, pol_parent);
	}

	bone = &model->bones[model->nr_bones];
	ht_put_int(model->bone_map, pol_bone->id, bone);
	bone->name = strdup(pol_bone->name);
	bone->index = model->nr_bones;
	bone->parent = parent ? parent->index : -1;

	glm_quat_mat4(pol_bone->rotq, bone->inverse_bind_matrix);
	glm_translate(bone->inverse_bind_matrix, pol_bone->pos);

	model->nr_bones++;
	return bone;
}

static void destroy_bone(struct bone *bone)
{
	free(bone->name);
}

struct model *model_load(struct archive *aar, const char *path, struct RE_renderer *r)
{
	if (path[0] == '\0')
		return NULL;

	const char *basename = strrchr(path, '\\');
	basename = basename ? basename + 1 : path;

	// Load .POL file
	struct archive_data *pol_file = get_aar_entry(aar, path, basename, ".POL");
	if (!pol_file) {
		WARNING("%s\\%s.POL: not found", path, basename);
		return NULL;
	}
	struct pol *pol = pol_parse(pol_file->data, pol_file->size);
	if (!pol) {
		WARNING("%s: parse error", pol_file->name);
		archive_free_data(pol_file);
		return NULL;
	}
	archive_free_data(pol_file);

	// Load .amt file, if any
	struct amt *amt = NULL;
	struct archive_data *amt_file = get_aar_entry(aar, path, basename, ".amt");
	if (amt_file) {
		amt = amt_parse(amt_file->data, amt_file->size);
		if (!amt)
			WARNING("%s: parse error", amt_file->name);
		archive_free_data(amt_file);
	}

	struct model *model = xcalloc(1, sizeof(struct model));
	model->path = strdup(path);

	// Bones
	if (pol->nr_bones > 0) {
		if (pol->nr_bones > MAX_BONES)
			ERROR("%s: Too many bones (%u)", model->path, pol->nr_bones);
		model->bone_map = ht_create(pol->nr_bones * 3 / 2);
		model->bones = xcalloc(pol->nr_bones, sizeof(struct bone));
		for (uint32_t i = 0; i < pol->nr_bones; i++) {
			add_bone(model, pol, &pol->bones[i]);
		}
		if (model->nr_bones != (int)pol->nr_bones)
			ERROR("%s: Broken bone data", model->path);
	}

	// Materials
	int *material_offsets = xmalloc(pol->nr_materials * sizeof(int));
	for (uint32_t i = 0; i < pol->nr_materials; i++) {
		material_offsets[i] = model->nr_materials;
		if (pol->materials[i].nr_children)
			model->nr_materials += pol->materials[i].nr_children;
		else
			model->nr_materials++;
	}
	model->materials = xcalloc(model->nr_materials, sizeof(struct material));
	for (uint32_t i = 0; i < pol->nr_materials; i++) {
		if (pol->materials[i].nr_children == 0) {
			init_material(&model->materials[material_offsets[i]],
				      &pol->materials[i].m, amt, aar, path);
			continue;
		}
		for (uint32_t j = 0; j < pol->materials[i].nr_children; j++) {
			init_material(&model->materials[material_offsets[i] + j],
				      &pol->materials[i].children[j], amt, aar, path);
		}
	}

	// Meshes
	for (uint32_t i = 0; i < pol->nr_meshes; i++) {
		if (!pol->meshes[i])
			continue;
		struct pol_material_group *mg = &pol->materials[pol->meshes[i]->material];
		int m_off = material_offsets[pol->meshes[i]->material];
		if (mg->nr_children == 0) {
			add_mesh(model, pol->meshes[i], -1, m_off, r);
			continue;
		}
		for (uint32_t j = 0; j < mg->nr_children; j++) {
			add_mesh(model, pol->meshes[i], j, m_off + j, r);
		}
	}

	free(material_offsets);
	if (amt)
		amt_free(amt);
	pol_free(pol);
	return model;
}

void model_free(struct model *model)
{
	for (int i = 0; i < model->nr_meshes; i++)
		destroy_mesh(&model->meshes[i]);
	free(model->meshes);

	for (int i = 0; i < model->nr_materials; i++)
		destroy_material(&model->materials[i]);
	free(model->materials);

	for (int i = 0; i < model->nr_bones; i++)
		destroy_bone(&model->bones[i]);
	free(model->bones);
	if (model->bone_map)
		ht_free_int(model->bone_map);

	free(model->path);
	free(model);
}

static int cmp_motions_by_bone_id(const void *lhs, const void *rhs)
{
	return (*(struct mot_bone **)lhs)->id - (*(struct mot_bone **)rhs)->id;
}

struct motion *motion_load(const char *name, struct RE_instance *instance, struct archive *aar)
{
	struct model *model = instance->model;
	if (!model)
		return NULL;

	struct archive_data *dfile = get_aar_entry(aar, model->path, name, ".MOT");
	if (!dfile) {
		WARNING("%s\\%s.MOT: not found", model->path, name);
		return NULL;
	}
	struct mot *mot = mot_parse(dfile->data, dfile->size);
	if (!mot) {
		WARNING("%s: parse error", dfile->name);
		archive_free_data(dfile);
		return NULL;
	}
	archive_free_data(dfile);

	if (model->nr_bones != (int)mot->nr_bones)
		ERROR("%s: wrong number of bones. Expected %d but got %d", name, model->nr_bones, mot->nr_bones);

	// Reorder mot->motions so that motion for model->bones[i] can be
	// accessed by mot->motions[i].
	for (uint32_t i = 0; i < mot->nr_bones; i++) {
		struct bone *bone = ht_get_int(model->bone_map, mot->motions[i]->id, NULL);
		if (!bone)
			ERROR("%s: invalid bone id %d", name, mot->motions[i]->id);
		mot->motions[i]->id = bone->index;
	}
	qsort(mot->motions, mot->nr_bones, sizeof(struct mot_bone *), cmp_motions_by_bone_id);

	struct motion *motion = xcalloc(1, sizeof(struct motion));
	motion->instance = instance;
	motion->mot = mot;
	motion->name = strdup(name);
	return motion;
}

void motion_free(struct motion *motion)
{
	if (motion->mot)
		mot_free(motion->mot);
	if (motion->name)
		free(motion->name);
	free(motion);
}
