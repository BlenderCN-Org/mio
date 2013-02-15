#include "mio.h"

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	memset(scene, 0, sizeof(*scene));
	return scene;
}

struct armature *new_armature(struct scene *scene, const char *skelname)
{
	struct armature *amt = malloc(sizeof(*amt));
	memset(amt, 0, sizeof(*amt));

	amt->skel = load_skel(skelname);
	mat_identity(amt->transform);

	list_insert(scene->armatures, amt);
	return amt;
}

struct object *new_object(struct scene *scene, const char *meshname)
{
	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));

	obj->mesh = load_mesh(meshname);
	mat_identity(obj->transform);

	list_insert(scene->objects, obj);
	return obj;
}

struct light *new_light(struct scene *scene)
{
	struct light *light = malloc(sizeof(*light));
	memset(light, 0, sizeof(*light));

	list_insert(scene->lights, light);
	return light;
}

void draw_object(struct scene *scene, struct object *obj, mat4 projection, mat4 view)
{
	mat4 model_view;
	mat_mul(model_view, view, obj->transform);
	draw_model(obj->mesh, projection, model_view);
}

void draw_scene(struct scene *scene, mat4 projection, mat4 view)
{
	struct armature *amt;
	struct object *obj;

	list_for_each(scene->objects, obj) {
		draw_object(scene, obj, projection, view);
	}

	list_for_each(scene->armatures, amt) {
	}
}
