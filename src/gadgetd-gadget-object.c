/*
 * gadgetd-gadget-object.c
 * Copyright(c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <glib-object.h>

#include <gio/gio.h>
#include <glib/gprintf.h>

#include <gadgetd-gadget-object.h>
#include <gadgetd-gdbus-codegen.h>
#include <gadgetd-common.h>
#include <gadget-strings.h>
#include <gadget-descriptors.h>
#include <gadget-function-manager.h>
#include <gadget-config-manager.h>

typedef struct _GadgetdGadgetObjectClass   GadgetdGadgetObjectClass;

struct _GadgetdGadgetObject
{
	GadgetdObjectSkeleton parent_instance;
	GadgetDaemon *daemon;

	gchar *gadget_path;
	struct gd_gadget *gadget;

	GadgetStrings *g_strings_iface;
	GadgetDescriptors *g_descriptors_iface;
	GadgetFunctionManager *g_func_manager_iface;
	GadgetConfigManager *g_cfg_manager_iface;
};

struct _GadgetdGadgetObjectClass
{
	GadgetdObjectSkeletonClass parent_class;
};


G_DEFINE_TYPE(GadgetdGadgetObject, gadgetd_gadget_object, GADGETD_TYPE_OBJECT_SKELETON);

enum
{
	PROPERTY,
	PROPERTY_DAEMON,
	PROPERTY_GADGET_PATH,
	PROPERTY_GADGET_PTR
};

/**
 * @brief object finalize
 * @param[in] object GObject
 */
static void
gadgetd_gadget_object_finalize(GObject *object)
{
	GadgetdGadgetObject *gadget_object = GADGETD_GADGET_OBJECT(object);

	g_free(gadget_object->gadget_path);

	g_free(gadget_object->gadget);

	if (gadget_object->g_strings_iface != NULL)
		g_object_unref(gadget_object->g_strings_iface);

	if (gadget_object->g_descriptors_iface != NULL)
		g_object_unref(gadget_object->g_descriptors_iface);

	if (gadget_object->g_func_manager_iface != NULL)
		g_object_unref(gadget_object->g_func_manager_iface);

	if (gadget_object->g_cfg_manager_iface != NULL)
		g_object_unref(gadget_object->g_cfg_manager_iface);

	if (G_OBJECT_CLASS(gadgetd_gadget_object_parent_class)->finalize != NULL)
		G_OBJECT_CLASS(gadgetd_gadget_object_parent_class)->finalize(object);
}

/**
 * @brief gadgetd gadget object get property.
 * @details  generic Getter for all properties of this type
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a GValue to return the property value in
 * @param[in] pspec the GParamSpec structure describing the property
 */
static void
gadgetd_gadget_object_get_property(GObject          *object,
                                    guint             property_id,
                                    GValue           *value,
                                    GParamSpec       *pspec)
{
	GadgetdGadgetObject *gadget_object = GADGETD_GADGET_OBJECT(object);

	switch(property_id) {
	case PROPERTY_DAEMON:
		g_value_set_object(value, gadgetd_gadget_object_get_daemon(gadget_object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd gadget object Set property
 * @param[in] object a GObject
 * @param[in] property_id numeric id under which the property was registered with
 * @param[in] value a new GValue for the property
 * @param[in] pspec the GParamSpec structure describing the property
 */
 void
gadgetd_gadget_object_set_property(GObject            *object,
				    guint              property_id,
				    const GValue      *value,
				    GParamSpec        *pspec)
{
	GadgetdGadgetObject *gadget_object = GADGETD_GADGET_OBJECT(object);

	switch(property_id) {
	case PROPERTY_DAEMON:
		g_assert(gadget_object->daemon == NULL);
		gadget_object->daemon = g_value_get_object(value);
		break;
	case PROPERTY_GADGET_PATH:
		g_assert(gadget_object->gadget_path == NULL);
		gadget_object->gadget_path = g_value_dup_string(value);
		break;
	case PROPERTY_GADGET_PTR:
		g_assert(gadget_object->gadget == NULL);
		gadget_object->gadget = g_value_get_pointer(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/**
 * @brief gadgetd gadget object init
 * @param[in] object GadgetdGadgetObject
 */

static void
gadgetd_gadget_object_init(GadgetdGadgetObject *object)
{
	/*nop*/
}

/**
 * @brief Gets the gd_gadget
 * @param[in] gadget_object GadgetdGadgetObject
 * @return pointer to gd_gadget, dont free
 */
struct gd_gadget *
gadgetd_gadget_object_get_gadget(GadgetdGadgetObject *gadget_object)
{
	g_return_val_if_fail(GADGETD_IS_GADGET_OBJECT(gadget_object), NULL);
	return gadget_object->gadget;
}

/**
 * @brief gadgetd gadget object get daemon object
 * @param[in] object GadgetdGadgetObject
 */
GadgetDaemon *
gadgetd_gadget_object_get_daemon(GadgetdGadgetObject *object)
{
	g_return_val_if_fail(GADGETD_GADGET_OBJECT(object), NULL);
	return object->daemon;
}

/**
 * @brief gadgetd gadget object constructed
 * @param[in] object GObject
 */
static void
gadgetd_gadget_object_constructed(GObject *object)
{
	GadgetdGadgetObject *gadget_object = GADGETD_GADGET_OBJECT(object);
	gchar *path = gadget_object->gadget_path;
	GadgetDaemon *daemon;

	daemon = gadgetd_gadget_object_get_daemon(gadget_object);

	/* add interfaces */
	gadget_object->g_strings_iface = gadget_strings_new(gadget_object->gadget);

	get_iface(G_OBJECT(gadget_object),
		   GADGET_TYPE_STRINGS,
		   &gadget_object->g_strings_iface);

	gadget_object->g_descriptors_iface = gadget_descriptors_new(gadget_object->gadget);

	get_iface(G_OBJECT(gadget_object),
		   GADGET_TYPE_DESCRIPTORS,
		   &gadget_object->g_descriptors_iface);

	gadget_object->g_func_manager_iface = gadget_function_manager_new(daemon,
									  path,
									  gadget_object->gadget);

	get_iface(G_OBJECT(gadget_object),
		   GADGET_TYPE_FUNCTION_MANAGER,
		   &gadget_object->g_func_manager_iface);

	gadget_object->g_cfg_manager_iface = gadget_config_manager_new(daemon,
								       path,
								       gadget_object->gadget);

	get_iface(G_OBJECT(gadget_object),
		   GADGET_TYPE_CONFIG_MANAGER,
		   &gadget_object->g_cfg_manager_iface);

	if (path != NULL && g_variant_is_object_path(path) && gadget_object != NULL)
		g_dbus_object_skeleton_set_object_path(G_DBUS_OBJECT_SKELETON(gadget_object), path);
	else
		ERROR("Unexpected error during gadget object creation");

	if (G_OBJECT_CLASS(gadgetd_gadget_object_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(gadgetd_gadget_object_parent_class)->constructed(object);

}

/**
 * @brief gadgetd gadget object class init
 * @param[in] klass GadgetdGadgetObjectClass
 */
static void
gadgetd_gadget_object_class_init(GadgetdGadgetObjectClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gadgetd_gadget_object_finalize;
	gobject_class->constructed  = gadgetd_gadget_object_constructed;
	gobject_class->set_property = gadgetd_gadget_object_set_property;
	gobject_class->get_property = gadgetd_gadget_object_get_property;

	g_object_class_install_property(gobject_class,
                                   PROPERTY_DAEMON,
                                   g_param_spec_object("daemon",
                                                       "Daemon",
                                                       "daemon for obj",
                                                       GADGET_TYPE_DAEMON,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class,
                                   PROPERTY_GADGET_PATH,
                                   g_param_spec_string("gadget_path",
                                                       "Gadget path",
                                                       "gadget path",
                                                       NULL,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(gobject_class,
					PROPERTY_GADGET_PTR,
					g_param_spec_pointer("gd_gadget",
							    "Gadget ptr",
							    "gadget ptr",
							    G_PARAM_READABLE |
							    G_PARAM_WRITABLE |
							    G_PARAM_CONSTRUCT_ONLY));
}

/**
 * @brief gadgetd gadget object new
 * @param[in] daemon GadgetDaemon
 * @param[in] gadget_path Path where object should be exported
 * @return GadgetdGadgetObject object
 */
GadgetdGadgetObject *
gadgetd_gadget_object_new(GadgetDaemon *daemon, const gchar *gadget_path,
			  struct gd_gadget *g)
{
	g_return_val_if_fail(GADGET_IS_DAEMON(daemon), NULL);
	g_return_val_if_fail(gadget_path != NULL, NULL);

	GadgetdGadgetObject *object;
	object = g_object_new(GADGETD_TYPE_GADGET_OBJECT,
			     "daemon", daemon,
			     "gadget_path", gadget_path,
			      "gd_gadget", g,
			      NULL);

	return object;
}
