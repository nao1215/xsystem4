/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
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
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <ctype.h>
#include "system4.h"
#include "ain.h"
#include "ald.h"
#include "ini.h"

struct config config = {
	.game_name = NULL,
	.ain_filename = NULL,
	.view_width = 800,
	.view_height = 600
};

// XXX: need to remove quotes from ini strings
static char *config_strdup(const char *str)
{
	if (*str != '"')
		ERROR("Not a string: %s", str);
	char *out = strdup(str+1);
	size_t len = strlen(out);
	if (!len || out[len-1] != '"')
		ERROR("Not a string: %s", str);
	out[len-1] = '\0';
	return out;
}

static int config_handler(void *user, possibly_unused const char *section, const char *name, const char *value)
{
	struct config *config = (struct config*)user;
	if (!strcmp(name, "GameName")) {
		config->game_name = config_strdup(value);
	} else if (!strcmp(name, "CodeName")) {
		config->ain_filename = config_strdup(value);
	} else if (!strcmp(name, "ViewWidth")) {
		config->view_width = atoi(value);
	} else if (!strcmp(name, "ViewHeight")) {
		config->view_height = atoi(value);
	}
	return 1;
}

static char *ald_filenames[ALDFILETYPE_MAX][ALD_FILEMAX];
static int ald_count[ALDFILETYPE_MAX];

void ald_init(int type, char **files, int count)
{
	int error = ALD_SUCCESS;
	ald[type] = ald_open(files, count, ALD_MMAP, &error);
	if (error)
		ERROR("Failed to open ALD file: %s\n", ald_strerror(error));
}

static void init_gamedata_dir(const char *path)
{
	DIR *dir;
	struct dirent *d;
	char filepath[512] = { [511] = '\0' };

	if (!(dir = opendir(path))) {
		ERROR("Failed to open directory: %s", path);
	}

	// get ALD filenames
	while ((d = readdir(dir))) {
		int dno;
		size_t len = strlen(d->d_name);
		snprintf(filepath, 511, "%s/%s", path, d->d_name);
		if (strcasecmp(d->d_name+len-4, ".ald"))
			continue;
		dno = toupper(*(d->d_name+len-5)) - 'A';
		if (dno < 0 || dno >= ALD_FILEMAX)
			continue;

		switch (*(d->d_name+len-6)) {
		case 'b':
		case 'B':
			ald_filenames[ALDFILE_BGM][dno] = strdup(filepath);
			ald_count[ALDFILE_BGM] = max(ald_count[ALDFILE_BGM], dno+1);
			break;
		case 'g':
		case 'G':
			ald_filenames[ALDFILE_CG][dno] = strdup(filepath);
			ald_count[ALDFILE_CG] = max(ald_count[ALDFILE_CG], dno+1);
			break;
		case 'w':
		case 'W':
			ald_filenames[ALDFILE_WAVE][dno] = strdup(filepath);
			ald_count[ALDFILE_WAVE] = max(ald_count[ALDFILE_WAVE], dno+1);
			break;
		default:
			WARNING("Unhandled ALD file: %s", d->d_name);
			break;
		}
	}

	// open ALD archives
	if (ald_count[ALDFILE_BGM] > 0)
		ald_init(ALDFILE_BGM, ald_filenames[ALDFILE_BGM], ald_count[ALDFILE_BGM]);
	if (ald_count[ALDFILE_CG] > 0)
		ald_init(ALDFILE_CG, ald_filenames[ALDFILE_CG], ald_count[ALDFILE_CG]);
	if (ald_count[ALDFILE_WAVE] > 0)
		ald_init(ALDFILE_WAVE, ald_filenames[ALDFILE_WAVE], ald_count[ALDFILE_WAVE]);

}

int main(int argc, char *argv[])
{
	size_t len;
	char *dir, *ainfile;
	int err = AIN_SUCCESS;
	struct ain *ain;

	// TODO: if no argument given, try to read System40.ini/AliceStart.ini from current dir
	if (argc != 2) {
		ERROR("Wrong number of arguments");
		return 1;
	}

	len = strlen(argv[1]);
	if (len > 4 && !strcasecmp(&argv[1][len - 4], ".ini")) {
		if (ini_parse(argv[1], config_handler, &config) < 0)
			ERROR("Can't parse %s as ini", argv[1]);
		if (!config.ain_filename)
			ERROR("No AIN filename specified in %s", argv[1]);

		dir = dirname(argv[1]);
		ainfile = xmalloc(strlen(dir) + strlen(config.ain_filename) + 2);
		strcpy(ainfile, dir);
		strcat(ainfile, "/");
		strcat(ainfile, config.ain_filename);
	} else if (len > 4 && !strcasecmp(&argv[1][len - 4], ".ain")) {
		ainfile = strdup(argv[1]);
		dir = dirname(argv[1]);
	} else  {
		ERROR("Not an AIN/INI file: %s", &argv[1][len - 4]);
	}

	if (!config.game_name)
		config.game_name = strdup("Unknown game");

	if (!(ain = ain_open(ainfile, &err))) {
		ERROR("%s", ain_strerror(err));
	}

	init_gamedata_dir(dir);

	vm_execute_ain(ain);
	sys_exit(0);
}
