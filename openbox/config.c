#include "config.h"

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif

static void config_free_entry(ConfigEntry *entry);
static void config_set_entry(char *name, ConfigValueType type,
                             ConfigValue value);
static void config_def_free(ConfigDefEntry *entry);
static void print_config(GQuark q, gpointer data, gpointer fonk){
    ConfigDefEntry *e = (ConfigDefEntry *)data;
    g_message("config: %s %d", e->name, e->hasList);
}

static GData *config = NULL;
static GData *config_def = NULL;

/* provided by cparse.l */
void cparse_go(char *filename, FILE *);


void config_startup()
{
    /* set up built in variables! and their default values! */

    config_def_set(config_def_new("engine", Config_String));
    config_def_set(config_def_new("theme", Config_String));
    config_def_set(config_def_new("font", Config_String));
    config_def_set(config_def_new("font.shadow.offset", Config_Integer));
    config_def_set(config_def_new("font.shadow.tint", Config_Integer));
    config_def_set(config_def_new("titlebar.layout", Config_String));

    /*g_datalist_foreach(&config_def, print_config, NULL);*/
}

void config_shutdown()
{
    g_datalist_clear(&config);
    g_datalist_clear(&config_def);
}

void config_parse()
{
    FILE *file;
    char *path;
    gboolean load = FALSE;

    /* load the user rc */
    path = g_build_filename(g_get_home_dir(), ".openbox", "rc3", NULL);
    if ((file = fopen(path, "r")) != NULL) {
        cparse_go(path, file);
        fclose(file);
        load = TRUE;
    }
    g_free(path);
    g_free(path);

    if (!load) {
        /* load the system wide rc */
        path = g_build_filename(RCDIR, "rc3", NULL);
        if ((file = fopen(path, "r")) != NULL) {
            cparse_go(path, file);
            fclose(file);
        }
    }
}

gboolean config_set(char *name, ConfigValueType type, ConfigValue value)
{
    ConfigDefEntry *def;
    gboolean ret = FALSE;

    name = g_ascii_strdown(name, -1);
    g_message("Setting %s", name);

    /*g_datalist_foreach(&config_def, print_config, NULL);*/
    def = g_datalist_get_data(&config_def, name);

    if (def == NULL) {
        g_message("Invalid config option '%s'", name);
    } else {
        if (def->hasList) {
            gboolean found = FALSE;
            GSList *it;

            it = def->values;
            g_assert(it != NULL);
            do {
                if (g_ascii_strcasecmp(it->data, value.string) == 0) {
                    found = TRUE;
                    break;
                }
            } while ((it = it->next));

            if (!found)
                g_message("Invalid value '%s' for config option '%s'",
                          value.string, name);
            else
                ret = TRUE;
        } else
            ret = TRUE;

    }

    if (ret)
        config_set_entry(name, type, value);
    else
        g_free(name);

    return ret;
}

gboolean config_get(char *name, ConfigValueType type, ConfigValue *value)
{
    ConfigEntry *entry;
    gboolean ret = FALSE;

    name = g_ascii_strdown(name, -1);
    entry = g_datalist_get_data(&config, name);
    if (entry != NULL && entry->type == type) {
        *value = entry->value;
        ret = TRUE;
    }
    g_free(name);
    return ret;
}

static void config_set_entry(char *name, ConfigValueType type,
                             ConfigValue value)
{
    ConfigEntry *entry = NULL;

    entry = g_new(ConfigEntry, 1);
    entry->name = name;
    entry->type = type;
    if (type == Config_String)
        entry->value.string = g_strdup(value.string);
    else
        entry->value = value;

    g_datalist_set_data_full(&config, name, entry,
                             (GDestroyNotify)config_free_entry);
}

static void config_free_entry(ConfigEntry *entry)
{
    g_free(entry->name);
    entry->name = NULL;
    if(entry->type == Config_String) {
        g_free(entry->value.string);
        entry->value.string = NULL;
    }
    g_free(entry);
}

ConfigDefEntry *config_def_new(char *name, ConfigValueType type)
{
    ConfigDefEntry *entry;

    entry = g_new(ConfigDefEntry, 1);
    entry->name = g_ascii_strdown(name, -1);
    entry->hasList = FALSE;
    entry->type = type;
    entry->values = NULL;
    return entry;
}

static void config_def_free(ConfigDefEntry *entry)
{
    GSList *it;

    g_free(entry->name);
    if (entry->hasList) {
        for (it = entry->values; it != NULL; it = it->next)
            g_free(it->data);
        g_slist_free(entry->values);
    }
    g_free(entry);
}

gboolean config_def_add_value(ConfigDefEntry *entry, char *value)
{
    if (entry->type != Config_String) {
        g_warning("Tried adding value to non-string config definition");
        return FALSE;
    }

    entry->hasList = TRUE;
    entry->values = g_slist_append(entry->values, g_ascii_strdown(value, -1));
    return TRUE;
}

gboolean config_def_set(ConfigDefEntry *entry)
{
    gboolean ret = FALSE;
    ConfigDefEntry *def;

    if ((def = g_datalist_get_data(&config_def, entry->name))) {
        g_assert(def != entry); /* adding it twice!? */
        g_warning("Definition already set for config option '%s'. ",
                  entry->name);
        config_def_free(entry);
    } else {
        g_datalist_set_data_full(&config_def, entry->name, entry,
                                 (GDestroyNotify)config_def_free);
        ret = TRUE;
    }

    return ret;
}
