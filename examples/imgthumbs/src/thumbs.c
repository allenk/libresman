#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <imago2.h>
#include "opengl.h"
#include "thumbs.h"
#include "resman.h"

#ifndef GL_COMPRESSED_RGB
#define GL_COMPRESSED_RGB	0x84ed
#endif

struct resman *texman;

static int load_res_texture(const char *fname, int id, void *cls);
static int done_res_texture(int id, void *cls);
static void free_res_texture(int id, void *cls);


struct thumbnail *create_thumbs(const char *dirpath)
{
	DIR *dir;
	struct dirent *dent;
	struct thumbnail *list = 0;

	/*unsigned int intfmt = GL_COMPRESSED_RGB;
	if(!strstr((char*)glGetString(GL_EXTENSIONS), "GL_ARB_texture_compression")) {
		printf("warning, no texture compression available.\n");
		intfmt = GL_RGB;
	}*/

	if(!texman) {
		texman = resman_create();
		resman_set_load_func(texman, load_res_texture, 0);
		resman_set_done_func(texman, done_res_texture, 0);
		resman_set_destroy_func(texman, free_res_texture, 0);
	}

	if(!(dir = opendir(dirpath))) {
		fprintf(stderr, "failed to open directory: %s: %s\n", dirpath, strerror(errno));
		return 0;
	}

	while((dent = readdir(dir))) {
		/*int xsz, ysz;
		unsigned char *pixels;*/
		struct thumbnail *node;

		if(!(node = malloc(sizeof *node))) {
			perror("failed to allocate thumbnail list node");
			continue;
		}
		memset(node, 0, sizeof *node);

		if(!(node->fname = malloc(strlen(dirpath) + strlen(dent->d_name) + 2))) {
			free(node);
			continue;
		}
		strcpy(node->fname, dirpath);
		if(dirpath[strlen(dirpath) - 1] != '/') {
			strcat(node->fname, "/");
		}
		strcat(node->fname, dent->d_name);

		node->aspect = 1.0;/*(float)xsz / (float)ysz;*/

		resman_lookup(texman, node->fname, 0);

		/*if(!(pixels = img_load_pixels(node->fname, &xsz, &ysz, IMG_FMT_RGBA32))) {
			free(node->fname);
			free(node);
			continue;
		}

		printf("loaded image: %s\n", node->fname);

		glGenTextures(1, &node->tex);
		glBindTexture(GL_TEXTURE_2D, node->tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, intfmt, xsz, ysz, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		img_free_pixels(pixels);
		*/

		node->next = list;
		list = node;
	}
	closedir(dir);

	return list;
}

void free_thumbs(struct thumbnail *thumbs)
{
	if(!thumbs) return;

	while(thumbs) {
		struct thumbnail *tmp = thumbs;
		thumbs = thumbs->next;

		free(tmp->fname);
		free(tmp);
	}
}


void update_thumbs(void)
{
	resman_poll(texman);
}

void draw_thumbs(struct thumbnail *thumbs, float thumb_sz, float start_y)
{
	int vp[4];
	float gap = thumb_sz / 4.0;
	float x = gap;
	float y = gap + start_y;
	float view_aspect;

	glGetIntegerv(GL_VIEWPORT, vp);
	view_aspect = (float)(vp[2] - vp[0]) / (vp[3] - vp[1]);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1.0 / view_aspect, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);

	while(thumbs) {
		if(!thumbs->tex) {
			thumbs = thumbs->next;
			continue;
		}

		glPushMatrix();
		glTranslatef(x, y, 0);

		glScalef(thumb_sz, thumb_sz, 1);

		glBegin(GL_QUADS);
		glColor3f(0.25, 0.25, 0.25);
		glTexCoord2f(0, 0); glVertex2f(0, 0);
		glTexCoord2f(1, 0); glVertex2f(1, 0);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glEnd();

		if(thumbs->aspect >= 1.0) {
			glTranslatef(0, 0.5 - 0.5 / thumbs->aspect, 0);
			glScalef(1, 1.0 / thumbs->aspect, 1);
		} else {
			glTranslatef(0.5 - thumbs->aspect / 2.0, 0, 0);
			glScalef(thumbs->aspect, 1, 1);
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, thumbs->tex);

		glBegin(GL_QUADS);
		glColor3f(1, 1, 1);
		glTexCoord2f(0, 0); glVertex2f(0, 0);
		glTexCoord2f(1, 0); glVertex2f(1, 0);
		glTexCoord2f(1, 1); glVertex2f(1, 1);
		glTexCoord2f(0, 1); glVertex2f(0, 1);
		glEnd();

		glPopMatrix();
		glDisable(GL_TEXTURE_2D);

		thumbs->layout_pos[0] = x;
		thumbs->layout_pos[1] = y;
		thumbs->layout_size[0] = thumb_sz;
		thumbs->layout_size[1] = thumb_sz;

		x += thumb_sz + gap;
		if(x >= 1.0 - thumb_sz) {
			x = gap;
			y += thumb_sz + gap;
		}

		thumbs = thumbs->next;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

static int load_res_texture(const char *fname, int id, void *cls)
{
	struct resman *rman = cls;
	struct thumbnail *rdata = resman_get_res_data(rman, id);

	assert(rdata);
	if(!rdata->img) {
		if(!(rdata->img = img_create())) {
			return -1;
		}
	}

	if(!img_load(rdata->img, fname) == -1) {
		img_free(rdata->img);
		return -1;
	}
	rdata->aspect = (float)rdata->img->width / (float)rdata->img->height;

	/* set the resource's data to the loaded image, so that we can use
	 * it in the done callback */
	resman_set_res_data(rman, id, rdata);
	return 0;
}

static int done_res_texture(int id, void *cls)
{
	struct thumbnail *rdata;
	struct resman *rman = cls;

	rdata = resman_get_res_data(rman, id);

	if(resman_get_res_result(rman, id) != 0 || !rdata) {
		fprintf(stderr, "failed to load resource %d (%s)\n", id, resman_get_res_name(rman, id));
	} else {
		printf("done loading resource %d (%s)\n", id, resman_get_res_name(rman, id));
	}

	if(!rdata->tex) {
		glGenTextures(1, &rdata->tex);
	}
	glBindTexture(GL_TEXTURE_2D, rdata->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, img_glintfmt(rdata->img),
			rdata->img->width, rdata->img->height, 0, img_glfmt(rdata->img),
			img_gltype(rdata->img), rdata->img->pixels);
	return 0;
}

static void free_res_texture(int id, void *cls)
{
	struct resman *rman = cls;
	struct thumbnail *rdata = resman_get_res_data(rman, id);

	if(rdata) {
		if(rdata->tex) {
			glDeleteTextures(1, &rdata->tex);
		}
		if(rdata->img) {
			img_free(rdata->img);
		}
	}
}
