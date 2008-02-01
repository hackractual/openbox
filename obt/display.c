/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   obt/display.c for the Openbox window manager
   Copyright (c) 2007-2008   Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include "obt/display.h"
#include "obt/prop.h"
#include "obt/internal.h"
#include "obt/keyboard.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

Display* obt_display = NULL;

gboolean obt_display_error_occured = FALSE;

gboolean obt_display_extension_xkb       = FALSE;
gint     obt_display_extension_xkb_basep;
gboolean obt_display_extension_shape     = FALSE;
gint     obt_display_extension_shape_basep;
gboolean obt_display_extension_xinerama  = FALSE;
gint     obt_display_extension_xinerama_basep;
gboolean obt_display_extension_randr     = FALSE;
gint     obt_display_extension_randr_basep;
gboolean obt_display_extension_sync      = FALSE;
gint     obt_display_extension_sync_basep;
gboolean obt_display_extension_composite = FALSE;
gint     obt_display_extension_composite_basep;
gboolean obt_display_extension_damage    = FALSE;
gint     obt_display_extension_damage_basep;
gboolean obt_display_extension_render    = FALSE;
gint     obt_display_extension_render_basep;
gboolean obt_display_extension_fixes     = FALSE;
gint     obt_display_extension_fixes_basep;

static gint xerror_handler(Display *d, XErrorEvent *e);

static gboolean xerror_ignore = FALSE;

gboolean obt_display_open(const char *display_name)
{
    gchar *n;
    Display *d = NULL;

    n = display_name ? g_strdup(display_name) : NULL;
    obt_display = d = XOpenDisplay(n);
    if (d) {
        gint junk;
        (void)junk;

        if (fcntl(ConnectionNumber(d), F_SETFD, 1) == -1)
            g_message("Failed to set display as close-on-exec");
        XSetErrorHandler(xerror_handler);

        /* read what extensions are present */
#ifdef XKB
        obt_display_extension_xkb =
            XkbQueryExtension(d, &junk,
                              &obt_display_extension_xkb_basep, &junk,
                              NULL, NULL);
        if (!obt_display_extension_xkb)
            g_message("XKB extension is not present on the server");
#endif

#ifdef SHAPE
        obt_display_extension_shape =
            XShapeQueryExtension(d, &obt_display_extension_shape_basep,
                                 &junk);
        if (!obt_display_extension_shape)
            g_message("X Shape extension is not present on the server");
#endif

#ifdef XINERAMA
        obt_display_extension_xinerama =
            XineramaQueryExtension(d,
                                   &obt_display_extension_xinerama_basep,
                                   &junk) && XineramaIsActive(d);
        if (!obt_display_extension_xinerama)
            g_message("Xinerama extension is not present on the server");
#endif

#ifdef XRANDR
        obt_display_extension_randr =
            XRRQueryExtension(d, &obt_display_extension_randr_basep,
                              &junk);
        if (!obt_display_extension_randr)
            g_message("XRandR extension is not present on the server");
#endif

#ifdef SYNC
        obt_display_extension_sync =
            XSyncQueryExtension(d, &obt_display_extension_sync_basep,
                                &junk) && XSyncInitialize(d, &junk, &junk);
        if (!obt_display_extension_sync)
            g_message("X Sync extension is not present on the server");
#endif

#ifdef USE_COMPOSITING
        if (XCompositeQueryExtension(d, &obt_display_extension_composite_basep,
                                     &junk))
        {
            gint major = 0, minor = 2;
            XCompositeQueryVersion(d, &major, &minor);
            /* Version 0.2 is the first version to have the
               XCompositeNameWindowPixmap() request. */
            if (major > 0 || minor >= 2)
                obt_display_extension_composite = TRUE;
        }
        if (!obt_display_extension_composite)
            g_message("X Composite extension is not present on the server or "
                      "is an incompatible version");

        obt_display_extension_damage =
            XDamageQueryExtension(d, &obt_display_extension_damage_basep,
                                  &junk);
        if (!obt_display_extension_damage)
            g_message("X Damage extension is not present on the server");

        obt_display_extension_render =
            XRenderQueryExtension(d, &obt_display_extension_render_basep,
                                  &junk);
        if (!obt_display_extension_render)
            g_message("X Render extension is not present on the server");

        obt_display_extension_fixes =
            XFixesQueryExtension(d, &obt_display_extension_fixes_basep,
                                  &junk);
        if (!obt_display_extension_fixes)
            g_message("X Fixes extension is not present on the server");
#endif

        obt_prop_startup();
        obt_keyboard_reload();
    }
    g_free(n);

    return obt_display != NULL;
}

void obt_display_close()
{
    obt_keyboard_shutdown();
    if (obt_display) XCloseDisplay(obt_display);
}

static gint xerror_handler(Display *d, XErrorEvent *e)
{
#ifdef DEBUG
    gchar errtxt[128];

    XGetErrorText(d, e->error_code, errtxt, 127);
    if (!xerror_ignore) {
        if (e->error_code == BadWindow)
            /*g_message(_("X Error: %s\n"), errtxt)*/;
        else
            g_error("X Error: %s", errtxt);
    } else
        g_message("Ignoring XError code %d '%s'", e->error_code, errtxt);
#else
    (void)d; (void)e;
#endif

    obt_display_error_occured = TRUE;
    return 0;
}

void obt_display_ignore_errors(gboolean ignore)
{
    XSync(obt_display, FALSE);
    xerror_ignore = ignore;
    if (ignore) obt_display_error_occured = FALSE;
}
