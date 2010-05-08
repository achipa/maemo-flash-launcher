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

#ifndef _NPPLAYER_H_
#define _NPPLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define XP_UNIX
#define MOZ_X11
#include "moz-sdk/npupp.h"

typedef const char* (* NP_LOADDS NP_GetMIMEDescriptionUPP)();
typedef NPError (* NP_InitializeUPP)(NPNetscapeFuncs*, NPPluginFuncs*);
typedef NPError (* NP_ShutdownUPP)(void);

typedef struct _NPPluginLib {
    char *library;
    GModule *module;
    NP_GetMIMEDescriptionUPP npGetMIMEDescription;
    NP_InitializeUPP npInitialize;
    NP_ShutdownUPP npShutdown;
} NPPluginLib;


typedef struct _NSNotify {
    void (* setDimension) (void *ndata, int w, int h);
    void (* getUrl) (void *inst, uint32_t stream,
            const char *url, const char *target,
            unsigned post_len, const char *post_data, void *ndata);
    char *(*evaluate) (const char *, void *ndata);
    void (* finishStream) (void *inst, uint32_t stream, void *ndata);
} NSNotify;

NPPluginLib *nppPluginInit (const char *plugin_lib);
void nppPluginShutdown (NPPluginLib *lib);

void nppStreamData (uint32_t stream, const char *buf, uint32_t sz, void *pdata);
void nppStreamFinished (uint32_t stream, uint32_t reason, void *pdata);
void nppStreamRedirected (uint32_t stream, const char *url);
void nppStreamInfo (uint32_t stream, const char *mime, unsigned long length);

void *nppInstanceOpen (const char *mime, uint16_t argc,
        char *argn[], char *argv[], void *ndata, NSNotify *notify);
void nppInstanceWindowParent (void *inst, void * wid);
void nppInstanceTopUrl (const char *url);
void nppInstanceStart (const char *url, const char *mime, void *inst);
void nppInstanceWindowDimension (void *inst, int x, int y, int w, int h);
void nppInstanceClose (void *inst);
void nppInstanceWindowClose (void *inst);

#ifdef __cplusplus
}
#endif

#endif //_NPPLAYER_H_
