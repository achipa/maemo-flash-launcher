/*
* Copyright (C) 2007  Koos Vriezen <koos.vriezen@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
http://devedge-temp.mozilla.org/library/manuals/2002/plugin/1.0/
*/

#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#include <glib/gprintf.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#define XP_UNIX
#define MOZ_X11
#include "moz-sdk/npupp.h"

#include "npplayer.h"

static int top_w, top_h;
static Window socket_id;
static int update_dimension_timer;
static gchar *start_url;

static void *browser_data;
static NSNotify *browser_notify;
static NPNetscapeFuncs ns_funcs;
static NPPluginFuncs np_funcs;       /* plugin functions              */
static NPP npp;                      /* single instance of the plugin */
static NPWindow np_window;
static NPObject *js_window;
static NPObject *scriptable_peer;
static NPSavedData *saved_data;
static NPClass js_class;
static NPClass window_class;
static NPClass location_class;
static GTree *stream_list;
static int stream_id_counter;
static GTree *identifiers;
typedef struct _StreamInfo {
    NPStream np_stream;
    /*unsigned int stream_buf_pos;*/
    unsigned int stream_pos;
    unsigned int total;
    unsigned int reason;
    unsigned int post_len;
    char *url;
    char *mimetype;
    char *target;
    char *post;
    bool notify;
    bool called_plugin;
    bool destroyed;
} StreamInfo;
struct JsObject;
typedef struct _JsObject {
    NPObject npobject;
    char * name;
} JsObject;

/*----------------%<---------------------------------------------------------*/

static void print (const char * format, ...) {
    va_list vl;
    va_start (vl, format);
    vfprintf (stderr, format, vl);
    va_end (vl);
    fflush (stderr);
}

static void *nsAlloc (uint32 size) {
    return g_malloc (size);
}

static void nsMemFree (void* ptr) {
    g_free (ptr);
}

/*----------------%<---------------------------------------------------------*/

static gint streamCompare (gconstpointer a, gconstpointer b) {
    return (long)a - (long)b;
}

static void freeStream (StreamInfo *si) {
    if (!g_tree_remove (stream_list, si->np_stream.ndata))
        print ("WARNING freeStream not in tree\n");
    g_free (si->url);
    if (si->mimetype)
        g_free (si->mimetype);
    if (si->target)
        g_free (si->target);
    if (si->post)
        g_free (si->post);
    nsMemFree (si);
}

static gboolean requestStream (void * p) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, p);
    if (si) {
        browser_notify->getUrl (npp, (int)(long)p, si->url, si->target,
                si->post_len, si->post, browser_data);
    } else {
        print ("requestStream %d not found", (long) p);
    }
    return 0; /* single shot */
}

static gboolean destroyStream (void * p) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, p);
    print ("FIXME destroyStream %d\n", (int)p);
    if (si)
        browser_notify->finishStream (npp, (int)(long)p, browser_data);
    return 0; /* single shot */
}

static void removeStream (uint32_t stream) {
    StreamInfo *si = (StreamInfo*)g_tree_lookup (stream_list, (gpointer)stream);

    if (si) {
        print ("removeStream %d rec:%d reason:%d\n", stream, si->stream_pos, si->reason);
        if (!si->destroyed) {
            if (si->called_plugin && !si->target)
                np_funcs.destroystream (npp, &si->np_stream, si->reason);
            if (si->notify)
                np_funcs.urlnotify (npp,
                        si->url, si->reason, si->np_stream.notifyData);
        }
        freeStream (si);
    }
}

static int32_t writeStream (uint32_t stream, const char *buf, uint32_t count) {
    int32_t sz = -1;
    StreamInfo *si = (StreamInfo*)g_tree_lookup (stream_list, (gpointer)stream);
    /*print ("writeStream found %d count %d\n", !!si, count);*/
    if (si) {
        if (si->reason > NPERR_NO_ERROR) {
            sz = count; /* stream closed, skip remainings */
        } else {
            if (!si->called_plugin) {
                NPError err;
                uint16 stype = NP_NORMAL;
                err = np_funcs.newstream (npp, si->mimetype ? si->mimetype : "text/plain", &si->np_stream, 0, &stype);
                if (err != NPERR_NO_ERROR) {
                    g_printerr ("newstream error %d\n", err);
                    destroyStream (stream);
                    return count;
                }
                print ("newStream %d type:%d %s mime:%s\n", (long) stream, stype, si->url, si->mimetype ? si->mimetype : "text/plain");
                si->called_plugin = true;
            }
            if (count) /* urls with a target returns zero bytes */
                sz = np_funcs.writeready (npp, &si->np_stream);
            if (sz > 0) {
                sz = np_funcs.write (npp, &si->np_stream, si->stream_pos,
                        (int32_t) count > sz ? sz : (int32_t) count, buf);
                if (sz < 0) { /*FIXME plugin destroys stream here*/
                    destroyStream ((gpointer)stream);
                    return 0;
                }
            } else {
                sz = 0;
            }
            si->stream_pos += sz;
        }
    }
    return sz;
}

static StreamInfo *addStream (const char *url, const char *mime,
        const char *target, int len, const char *post, void *notify_data,
        bool notify) {
    StreamInfo *si = (StreamInfo *) nsAlloc (sizeof (StreamInfo));

    memset (si, 0, sizeof (StreamInfo));
    si->url = g_strdup (url);
    si->np_stream.url = si->url;
    if (mime)
        si->mimetype = g_strdup (mime);
    if (target)
        si->target = g_strdup (target);
    if (len && post) {
        si->post_len = len;
        si->post = (char *) nsAlloc (len);
        memcpy (si->post, post, len);
    }
    si->np_stream.notifyData = notify_data;
    si->notify = notify;
    si->np_stream.ndata = (void *) (long) (stream_id_counter++);
    print ("add stream %d\n", (long) si->np_stream.ndata);
    g_tree_insert (stream_list, si->np_stream.ndata, si);

    g_timeout_add (0, requestStream, si->np_stream.ndata);

    return si;
}

/*----------------%<---------------------------------------------------------*/

static NPObject * nsCreateObject (NPP instance, NPClass *aClass) {
    NPObject *obj;
    if (aClass && aClass->allocate)
        obj = aClass->allocate (instance, aClass);
    else
        obj = js_class.allocate (instance, &js_class);/*add null class*/
    /*print ("NPN_CreateObject\n");*/
    obj->referenceCount = 1;
    return obj;
}

static NPObject *nsRetainObject (NPObject *npobj) {
    /*print( "nsRetainObject %p\n", npobj);*/
    npobj->referenceCount++;
    return npobj;
}

static void nsReleaseObject (NPObject *npobj) {
    NPObject *obj = npobj;
    /*print ("NPN_ReleaseObject\n");*/
    if (!obj)
        obj = scriptable_peer;
    if (! (--obj->referenceCount)) {
        if (obj->_class->deallocate)
            obj->_class->deallocate (obj);
        else
            nsMemFree (obj);
        if (obj == scriptable_peer) {
            print ("NPN_ReleaseObject default\n");
            scriptable_peer = NULL;
        }
    }
}

static NPError nsGetURL (NPP instance, const char* url, const char* target) {
    (void)instance;
    print ("nsGetURL %s %s\n", url, target);
    addStream (url, 0L, target, 0, NULL, 0L, false);
    return NPERR_NO_ERROR;
}

static NPError nsPostURL (NPP instance, const char *url,
        const char *target, uint32 len, const char *buf, NPBool file) {
    (void)instance; (void)file;
    print ("nsPostURL %s %s\n", url, target);
    addStream (url, 0L, target, len, buf, 0L, false);
    return NPERR_NO_ERROR;
}

static NPError nsRequestRead (NPStream *stream, NPByteRange *rangeList) {
    (void)stream; (void)rangeList;
    print ("nsRequestRead\n");
    return NPERR_NO_ERROR;
}

static NPError nsNewStream (NPP instance, NPMIMEType type,
        const char *target, NPStream **stream) {
    (void)instance; (void)type; (void)stream; (void)target;
    print ("nsNewStream\n");
    return NPERR_NO_ERROR;
}

static int32 nsWrite (NPP instance, NPStream* stream, int32 len, void *buf) {
    (void)instance; (void)len; (void)buf; (void)stream;
    print ("nsWrite\n");
    return 0;
}

static NPError nsDestroyStream (NPP instance, NPStream *stream, NPError reason) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, stream->ndata);
    (void)instance;
    print ("nsDestroyStream\n");
    if (si) {
        si->reason = reason;
        si->destroyed = true;
        g_timeout_add (0, destroyStream, stream->ndata);
        return NPERR_NO_ERROR;
    }
    return NPERR_NO_DATA;
}

static void nsStatus (NPP instance, const char* message) {
    (void)instance;
    print ("NPN_Status %s\n", message);
}

static const char* nsUserAgent (NPP instance) {
    (void)instance;
    print ("NPN_UserAgent\n");
    return "Mozilla/5.0 (X11; U; Linux armv7l; en-US; rv:1.9.2a1pre) Gecko/20090907 Firefox/3.5 Maemo Browser 1.4.1.8 RX-51 N900";
}

static uint32 nsMemFlush (uint32 size) {
    (void)size;
    print ("NPN_MemFlush\n");
    return 0;
}

static void nsReloadPlugins (NPBool reloadPages) {
    (void)reloadPages;
    print ("NPN_ReloadPlugins\n");
}

static JRIEnv* nsGetJavaEnv () {
    print ("NPN_GetJavaEnv\n");
    return NULL;
}

static jref nsGetJavaPeer (NPP instance) {
    (void)instance;
    print ("NPN_GetJavaPeer\n");
    return NULL;
}

static NPError nsGetURLNotify (NPP instance, const char* url, const char* target, void *notify) {
    (void)instance;
    addStream (url, 0L, target, 0, NULL, notify, true);
    return NPERR_NO_ERROR;
}

static NPError nsPostURLNotify (NPP instance, const char* url, const char* target, uint32 len, const char* buf, NPBool file, void *notify) {
    (void)instance; (void)file;
    print ("NPN_PostURLNotify\n");
    addStream (url, 0L, target, len, buf, notify, true);
    return NPERR_NO_ERROR;
}

static NPError nsGetValue (NPP instance, NPNVariable variable, void *value) {
    print ("NPN_GetValue %d\n", variable & ~NP_ABI_MASK);
    switch (variable) {
        case NPNVxDisplay:
            *(void**)value = (void*)(long) gdk_x11_get_default_xdisplay ();
            break;
        case NPNVxtAppContext:
            *(void**)value = NULL;
            break;
        case NPNVnetscapeWindow:
            print ("NPNVnetscapeWindow\n");
            *(int*)value = 0;
            break;
        case NPNVjavascriptEnabledBool:
            *(NPBool*)value = 1;
            break;
        case NPNVasdEnabledBool:
            *(NPBool*)value = 0;
            break;
        case NPNVisOfflineBool:
            *(NPBool*)value = 0;
            break;
        case NPNVserviceManager:
            *(int*)value = 0;
            break;
        case NPNVToolkit:
            *(int*)value = NPNVGtk2;
            break;
        case NPNVSupportsXEmbedBool:
            *(NPBool*)value = 1;
            break;
        case NPNVWindowNPObject:
            if (!js_window) {
                JsObject *jo = (JsObject*) nsCreateObject (instance, &window_class);
                jo->name = g_strdup ("window");
                js_window = (NPObject *) jo;
            }
            *(NPObject**)value = nsRetainObject (js_window);
            break;
        /*case NPNVPluginElementNPObject: {
            JsObject * obj = (JsObject *) nsCreateObject (instance, &js_class);
            obj->name = g_strdup ("this");
            *(NPObject**)value = (NPObject *) obj;
            break;
        }*/
        default:
            print ("unknown value\n");
            return NPERR_GENERIC_ERROR;
    }
    return NPERR_NO_ERROR;
}

static NPError nsSetValue (NPP instance, NPPVariable variable, void *value) {
    /* NPPVpluginWindowBool */
    (void)instance; (void)value;
    print ("NPN_SetValue %d\n", variable & ~NP_ABI_MASK);
    return NPERR_NO_ERROR;
}

static void nsInvalidateRect (NPP instance, NPRect *invalidRect) {
    (void)instance; (void)invalidRect;
    print ("NPN_InvalidateRect\n");
}

static void nsInvalidateRegion (NPP instance, NPRegion invalidRegion) {
    (void)instance; (void)invalidRegion;
    print ("NPN_InvalidateRegion\n");
}

static void nsForceRedraw (NPP instance) {
    (void)instance;
    print ("NPN_ForceRedraw\n");
}

static NPIdentifier nsGetStringIdentifier (const NPUTF8* name) {
    /*print ("NPN_GetStringIdentifier %s\n", name);*/
    gpointer id = g_tree_lookup (identifiers, name);
    if (!id) {
        id = g_strdup (name);
        g_tree_insert (identifiers, id, id);
    }
    return id;
}

static void nsGetStringIdentifiers (const NPUTF8** names, int32_t nameCount,
        NPIdentifier* ids) {
    (void)names; (void)nameCount; (void)ids;
    print ("NPN_GetStringIdentifiers\n");
}

static NPIdentifier nsGetIntIdentifier (int32_t intid) {
    print ("NPN_GetIntIdentifier %d\n", intid);
    return (NPIdentifier) (long) intid;
}

static bool nsIdentifierIsString (NPIdentifier name) {
    print ("NPN_IdentifierIsString\n");
    return !!g_tree_lookup (identifiers, name);
}

static NPUTF8 * nsUTF8FromIdentifier (NPIdentifier name) {
    char *str = g_tree_lookup (identifiers, name);
    print ("NPN_UTF8FromIdentifier\n");
    if (str)
        return (NPUTF8 *)g_strdup (str);
    return NULL;
}

static int32_t nsIntFromIdentifier (NPIdentifier identifier) {
    print ("NPN_IntFromIdentifier\n");
    return (int32_t) (long) identifier;
}

static bool nsInvoke (NPP instance, NPObject * npobj, NPIdentifier method,
        const NPVariant *args, uint32_t arg_count, NPVariant *result) {
    NPObject *obj = npobj;
    (void)instance;
    if (!obj) {
        if (!scriptable_peer &&
            NPERR_NO_ERROR != np_funcs.getvalue ((void*)npp,
                NPPVpluginScriptableNPObject, (void*)&scriptable_peer))
            return false;
        obj = scriptable_peer;
    }
    /*print ("NPN_Invoke %s\n", id);*/
    if (obj->_class->invoke)
        return obj->_class->invoke (obj, method, args, arg_count, result);
    return false;
}

static bool nsInvokeDefault (NPP instance, NPObject * npobj,
        const NPVariant * args, uint32_t arg_count, NPVariant * result) {
    (void)instance;
    if (npobj->_class->invokeDefault)
        return npobj->_class->invokeDefault (npobj,args, arg_count, result);
    return false;
}

static bool createUndefined (NPVariant *result) {
    result->type = NPVariantType_String;
    result->value.stringValue.utf8characters = g_strdup ("undefined");
    result->value.stringValue.utf8length = 9;
    return true;
}

static bool nsEvaluate (NPP instance, NPObject * npobj, NPString * script,
        NPVariant * result) {
    (void) instance;
    (void) script;
    return createUndefined (result);
}

static bool nsGetProperty (NPP instance, NPObject * npobj,
        NPIdentifier property, NPVariant * result) {
    (void)instance;
    if (npobj->_class->getProperty)
        return npobj->_class->getProperty (npobj, property, result);
    return false;
}

static bool nsSetProperty (NPP instance, NPObject * npobj,
        NPIdentifier property, const NPVariant *value) {
    (void)instance;
    if (npobj->_class->setProperty)
        return npobj->_class->setProperty (npobj, property, value);
    return false;
}

static bool nsRemoveProperty (NPP inst, NPObject * npobj, NPIdentifier prop) {
    (void)inst;
    if (npobj->_class->removeProperty)
        return npobj->_class->removeProperty (npobj, prop);
    return false;
}

static bool nsHasProperty (NPP instance, NPObject * npobj, NPIdentifier prop) {
    (void)instance;
    if (npobj->_class->hasProperty)
        return npobj->_class->hasProperty (npobj, prop);
    return false;
}

static bool nsHasMethod (NPP instance, NPObject * npobj, NPIdentifier method) {
    (void)instance;
    if (npobj->_class->hasMethod)
        return npobj->_class->hasMethod (npobj, method);
    return false;
}

static void nsReleaseVariantValue (NPVariant * variant) {
    /*print ("NPN_ReleaseVariantValue\n");*/
    switch (variant->type) {
        case NPVariantType_String:
            if (variant->value.stringValue.utf8characters)
                g_free ((char *) variant->value.stringValue.utf8characters);
            break;
        case NPVariantType_Object:
            if (variant->value.objectValue)
                nsReleaseObject (variant->value.objectValue);
            break;
        default:
            break;
    }
    variant->type = NPVariantType_Null;
}

static void nsSetException (NPObject *npobj, const NPUTF8 *message) {
    (void)npobj;
    print ("NPN_SetException %s\n", message);
}

static bool nsPushPopupsEnabledState (NPP instance, NPBool enabled) {
    (void)instance;
    print ("NPN_PushPopupsEnabledState %d\n", enabled);
    return false;
}

static bool nsPopPopupsEnabledState (NPP instance) {
    (void)instance;
    print ("NPN_PopPopupsEnabledState\n");
    return false;
}

/*----------------%<---------------------------------------------------------*/

static NPObject * jsClassAllocate (NPP instance, NPClass *aClass) {
    /*print ("jsClassAllocate\n");*/
    JsObject * jo = (JsObject *) nsAlloc (sizeof (JsObject));
    (void)instance;
    memset (jo, 0, sizeof (JsObject));
    jo->npobject._class = aClass;
    return (NPObject *) jo;
}

static void jsClassDeallocate (NPObject *npobj) {
    JsObject *jo = (JsObject *) npobj;
    /*print ("jsClassDeallocate\n");*/
    if (jo->name)
        g_free (jo->name);
    if (npobj == js_window) {
        print ("WARNING deleting window object\n");
        js_window = NULL;
    }
    nsMemFree (npobj);
}

static void jsClassInvalidate (NPObject *npobj) {
    (void)npobj;
    print ("jsClassInvalidate\n");
}

static bool jsClassHasMethod (NPObject *npobj, NPIdentifier name) {
    (void)npobj; (void)name;
    print ("windowClassHasMehtod\n");
    return false;
}

static bool jsClassInvoke (NPObject *npobj, NPIdentifier method,
        const NPVariant *args, uint32_t arg_count, NPVariant *result) {
    (void) npobj;
    (void) method;
    (void) args;
    (void) arg_count;
    return createUndefined (result);
}

static bool jsClassInvokeDefault (NPObject *npobj,
        const NPVariant *args, uint32_t arg_count, NPVariant *result) {
    (void)npobj; (void)args; (void)arg_count; (void)result;
    print ("jsClassInvokeDefault\n");
    return false;
}

static bool jsClassHasProperty (NPObject *npobj, NPIdentifier name) {
    (void)npobj; (void)name;
    print ("jsClassHasProperty\n");
    return false;
}

static bool jsClassGetProperty (NPObject *npobj, NPIdentifier property,
        NPVariant *result) {
    (void) npobj;
    (void) property;
    return createUndefined (result);
}

static bool jsClassSetProperty (NPObject *npobj, NPIdentifier property,
        const NPVariant *value) {
    (void) npobj;
    (void) property;
    (void) value;

    return true;
}

static bool jsClassRemoveProperty (NPObject *npobj, NPIdentifier name) {
    (void)npobj; (void)name;
    print ("jsClassRemoveProperty\n");
    return false;
}

static bool windowClassGetProperty (NPObject *npobj, NPIdentifier property,
        NPVariant *result) {
    char * id = (char *) g_tree_lookup (identifiers, property);
    print ("windowClassGetProperty.GetProperty %s\n", id);
    if (!strcmp (id, "location")) {
        JsObject *jo = (JsObject*) nsCreateObject (NULL, &location_class);
        jo->name = g_strdup ("location");
        result->value.objectValue = (NPObject *) jo;
        result->type = NPVariantType_Object;
        return true;
    } else if (!strcmp (id, "top")) {
        result->value.objectValue = nsRetainObject (npobj);
        result->type = NPVariantType_Object;
        return true;
    }
    return jsClassGetProperty (npobj, property, result);
}

static bool createStartLink (NPVariant *result) {
    result->type = NPVariantType_String;
    result->value.stringValue.utf8length = strlen (start_url);
    result->value.stringValue.utf8characters = nsAlloc (strlen (start_url) + 1);
    strcpy (result->value.stringValue.utf8characters, start_url);
    return true;
}

static bool locationClassGetProperty (NPObject *npobj, NPIdentifier property,
        NPVariant *result) {
    char * id = (char *) g_tree_lookup (identifiers, property);
    print ("locationClass.GetProperty %s\n", id);
    if (!strcmp (id, "href"))
        return createStartLink (result);
    return jsClassGetProperty (npobj, property, result);
}

static bool locationClassInvoke (NPObject *npobj, NPIdentifier method,
        const NPVariant *args, uint32_t arg_count, NPVariant *result) {
    char *id = (char *) g_tree_lookup (identifiers, method);
    print ("locationClass.Invoke %s\n", id);
    if (!strcmp (id, "toString"))
        return createStartLink (result);
    return jsClassInvoke (npobj, method, args, arg_count, result);
}


/*----------------%<---------------------------------------------------------*/

void nppPluginShutdown (NPPluginLib *lib) {
    print ("nppPluginShutdown\n");
    if (lib) {
        if (lib->npShutdown)
            lib->npShutdown();
        g_module_close (lib->module);
        nsMemFree (lib);
    }
}

NPPluginLib *nppPluginInit (const char *lib) {
    NPError np_err;
    NPPluginLib *plugin_lib;
    GModule *library;

    print ("starting %s\n", lib);
    library = g_module_open (lib, G_MODULE_BIND_LAZY);
    if (!library) {
        print ("failed to load %s %s\n", lib, g_module_error ());
        return NULL;
    }

    plugin_lib = (NPPluginLib *)nsAlloc (sizeof (NPPluginLib));
    plugin_lib->module = library;

    if (!g_module_symbol (library,
                "NP_GetMIMEDescription",
                (gpointer *)&plugin_lib->npGetMIMEDescription)) {
        print ("undefined reference to load NP_GetMIMEDescription\n");
        goto bail_out;
    }
    if (!g_module_symbol (library,
                "NP_Initialize", (gpointer *)&plugin_lib->npInitialize)) {
        print ("undefined reference to load NP_Initialize\n");
        goto bail_out;
    }
    if (!g_module_symbol (library,
                "NP_Shutdown", (gpointer *)&plugin_lib->npShutdown)) {
        print ("undefined reference to load NP_Shutdown\n");
        goto bail_out;
    }
    print ("startup succeeded %s\n", plugin_lib->npGetMIMEDescription ());

    memset (&ns_funcs, 0, sizeof (NPNetscapeFuncs));
    ns_funcs.size = sizeof (NPNetscapeFuncs);
    ns_funcs.version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
    ns_funcs.geturl = nsGetURL;
    ns_funcs.posturl = nsPostURL;
    ns_funcs.requestread = nsRequestRead;
    ns_funcs.newstream = nsNewStream;
    ns_funcs.write = nsWrite;
    ns_funcs.destroystream = nsDestroyStream;
    ns_funcs.status = nsStatus;
    ns_funcs.uagent = nsUserAgent;
    ns_funcs.memalloc = nsAlloc;
    ns_funcs.memfree = nsMemFree;
    ns_funcs.memflush = nsMemFlush;
    ns_funcs.reloadplugins = nsReloadPlugins;
    ns_funcs.getJavaEnv = nsGetJavaEnv;
    ns_funcs.getJavaPeer = nsGetJavaPeer;
    ns_funcs.geturlnotify = nsGetURLNotify;
    ns_funcs.posturlnotify = nsPostURLNotify;
    ns_funcs.getvalue = nsGetValue;
    ns_funcs.setvalue = nsSetValue;
    ns_funcs.invalidaterect = nsInvalidateRect;
    ns_funcs.invalidateregion = nsInvalidateRegion;
    ns_funcs.forceredraw = nsForceRedraw;
    ns_funcs.getstringidentifier = nsGetStringIdentifier;
    ns_funcs.getstringidentifiers = nsGetStringIdentifiers;
    ns_funcs.getintidentifier = nsGetIntIdentifier;
    ns_funcs.identifierisstring = nsIdentifierIsString;
    ns_funcs.utf8fromidentifier = nsUTF8FromIdentifier;
    ns_funcs.intfromidentifier = nsIntFromIdentifier;
    ns_funcs.createobject = nsCreateObject;
    ns_funcs.retainobject = nsRetainObject;
    ns_funcs.releaseobject = nsReleaseObject;
    ns_funcs.invoke = nsInvoke;
    ns_funcs.invokeDefault = nsInvokeDefault;
    ns_funcs.evaluate = nsEvaluate;
    ns_funcs.getproperty = nsGetProperty;
    ns_funcs.setproperty = nsSetProperty;
    ns_funcs.removeproperty = nsRemoveProperty;
    ns_funcs.hasproperty = nsHasProperty;
    ns_funcs.hasmethod = nsHasMethod;
    ns_funcs.releasevariantvalue = nsReleaseVariantValue;
    ns_funcs.setexception = nsSetException;
    ns_funcs.pushpopupsenabledstate = nsPushPopupsEnabledState;
    ns_funcs.poppopupsenabledstate = nsPopPopupsEnabledState;

    js_class.structVersion = NP_CLASS_STRUCT_VERSION;
    js_class.allocate = jsClassAllocate;
    js_class.deallocate = jsClassDeallocate;
    js_class.invalidate = jsClassInvalidate;
    js_class.hasMethod = jsClassHasMethod;
    js_class.invoke = jsClassInvoke;
    js_class.invokeDefault = jsClassInvokeDefault;
    js_class.hasProperty = jsClassHasProperty;
    js_class.getProperty = jsClassGetProperty;
    js_class.setProperty = jsClassSetProperty;
    js_class.removeProperty = jsClassRemoveProperty;

    window_class = js_class;
    window_class.getProperty = windowClassGetProperty;

    location_class = js_class;
    location_class.getProperty = locationClassGetProperty;
    location_class.invoke = locationClassInvoke;

    np_funcs.size = sizeof (NPPluginFuncs);

    np_err = plugin_lib->npInitialize (&ns_funcs, &np_funcs);
    if (np_err != NPERR_NO_ERROR) {
        print ("NP_Initialize failure %d\n", np_err);
        goto bail_out;
    }

    identifiers = g_tree_new (strcmp);
    stream_list = g_tree_new (streamCompare);

    return plugin_lib;

bail_out:
    nsMemFree (plugin_lib);
    return NULL;
}

void *nppInstanceOpen (const char *mime, uint16_t argc, char *argn[],
        char *argv[], void *ndata, NSNotify *notify) {
    NPSetWindowCallbackStruct ws_info;
    NPError np_err;
    Display *display;
    int screen;
    int i;
    int needs_xembed;
    /*unsigned int width = 0, height = 0;*/

    browser_data = ndata;
    browser_notify = notify;

    npp = (NPP_t*)nsAlloc (sizeof (NPP_t));
    memset (npp, 0, sizeof (NPP_t));
    /*for (i = 0; i < argc; i++) {
        print ("arg %s %s\n", argn[i], argv[i]);
        if (!strcasecmp (argn[i], "width"))
            width = strtol (argv[i], 0L, 10);
        else if (!strcasecmp (argn[i], "height"))
            height = strtol (argv[i], 0L, 10);
    }
    if (width > 0 && height > 0)
        notify->setDimension (ndata, width, height);*/

    np_err = np_funcs.newp (mime, npp, NP_EMBED, argc, argn, argv, saved_data);
    if (np_err != NPERR_NO_ERROR) {
        print ("NPP_New failure %d %p %p\n", np_err, np_funcs, np_funcs.newp);
        nsMemFree (npp);
        return NULL;
    }
    if (np_funcs.getvalue) {
        np_err = np_funcs.getvalue ((void*)npp,
                NPPVpluginNeedsXEmbed, (void*)&needs_xembed);
        if (np_err != NPERR_NO_ERROR || !needs_xembed) {
            print ("NPP_GetValue NPPVpluginNeedsXEmbed failure %d\n", np_err);
            /*shutdownPlugin();*/
            nsMemFree (npp);
            return NULL;
        }
    }
    memset (&np_window, 0, sizeof (NPWindow));
    display = gdk_x11_get_default_xdisplay ();
    np_window.x = 0;
    np_window.y = 0;
    np_window.width = top_w; /*width ? width : top_w;*/
    np_window.height = top_h; /*height ? height : top_h;*/
    np_window.window = (void*)socket_id;
    np_window.type = NPWindowTypeWindow;
    ws_info.type = NP_SETWINDOW;
    screen = DefaultScreen (display);
    ws_info.display = (void*)(long)display;
    ws_info.visual = (void*)(long)DefaultVisual (display, screen);
    ws_info.colormap = DefaultColormap (display, screen);
    ws_info.depth = DefaultDepth (display, screen);
    print ("display %u %dx%d\n", socket_id, np_window.width, np_window.height);
    np_window.ws_info = (void*)&ws_info;

    np_err = np_funcs.setwindow (npp, &np_window);

    return npp;
}

void nppInstanceTopUrl (const char *url) {
    g_free (start_url);
    start_url = g_strdup (url);
}

void nppInstanceStart (const char *url, const char *mime, void *pdata) {
    (void) pdata;
    addStream (url, mime, 0L, 0, NULL, 0L, false);
}

void nppStreamData (uint32_t stream, const char *buf, uint32_t sz, void *p) {
    int written = writeStream (stream, buf, sz);
    (void)p;
    if (written < sz)
        print ("FIXME: plugin didn't accept data %d written %d\n", sz, written);
}

void nppStreamFinished (uint32_t stream, uint32_t reason, void *pdata) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, (void *)stream);
    print ("nppStreamFinished\n");
    if (si) {
        si->reason = reason;
        removeStream (stream);
    }
}

void nppStreamRedirected (uint32_t stream, const char *url) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, stream);
    if (si) {
        g_free (si->url);
        si->url = g_strdup (url);
        si->np_stream.url = si->url;
    }
}

void nppStreamInfo (uint32_t sid, const char *mime, unsigned long length) {
    StreamInfo *si = (StreamInfo *) g_tree_lookup (stream_list, sid);
    if (si) {
        print ("nppStreamInfo %s %s\n", si->url, mime ? mime : "");
        if (si->mimetype)
            g_free (si->mimetype);
        si->mimetype = mime ? g_strdup (mime) : NULL;
        si->np_stream.end = length;
    }
}

void nppInstanceClose (void *pdata) {
    if (scriptable_peer) {
        nsReleaseObject (scriptable_peer);
        scriptable_peer = NULL;
    }
    if (npp) {
        np_funcs.destroy (npp, &saved_data);
        nsMemFree (npp);
        npp = 0L;
    }
    g_free (start_url);
    start_url = NULL;
    top_w = 0;
    top_h = 0;
    np_window.window = NULL;
}

void nppInstanceWindowClose (void *priv) {
    (void)priv;

    np_window.window = NULL;
}

/*----------------%<---------------------------------------------------------*/

static gboolean updateDimension (void * p) {
    (void)p;
    if (np_window.window &&
            (np_window.width != top_w || np_window.height != top_h)) {
        np_window.width = top_w;
        np_window.height = top_h;
        np_funcs.setwindow (npp, &np_window);
    }
    update_dimension_timer = 0;
    return 0; /* single shot */
}

void nppInstanceWindowDimension (void *p, int x, int y, int w, int h) {
    (void)p; (void) x; (void) y;
    print ("nppInstanceWindowDimension %dx%d\n", w, h);
    if (!update_dimension_timer)
      update_dimension_timer = g_timeout_add (100, updateDimension, NULL);
    top_w = w;
    top_h = h;
}

void nppInstanceWindowParent (void *p, void * wid) {
    (void)p;
    socket_id = (Window) wid;
}

