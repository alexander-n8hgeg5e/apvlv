/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CPPFILE ApvlvUtil.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2008/09/30 00:00:00 Alf */

#include "ApvlvUtil.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#ifndef WIN32
#include <sys/wait.h>
#endif
#ifdef WIN32
# include <windows.h>
#endif

#include <string>
#include <iostream>
using namespace std;

namespace apvlv
{

#ifdef WIN32
  string helppdf = "~\\Startup.pdf";
  string mainmenubar_glade = "~\\main_menubar.glade";
  string iniexam = "~\\apvlvrc.example";
  string iconreg = "~\\icons\\reg.png";
  string icondir = "~\\icons\\dir.png";
  string iconpdf = "~\\icons\\pdf.png";
  string inifile = "~\\_apvlvrc";
  string sessionfile = "~\\_apvlvinfo";
#else
  string helppdf = string (DOCDIR) + "/Startup.pdf";
  string mainmenubar_glade = string (LIBDIR) + "/main_menubar.glade";
  string iniexam = string (DOCDIR) + "/apvlvrc.example";
  string iconreg = string (PIXMAPDIR) + "/icons/reg.png";
  string icondir = string (PIXMAPDIR) + "/icons/dir.png";
  string iconpdf = string (PIXMAPDIR) + "/icons/pdf.png";
  string inifile = "~/.apvlvrc";
  string sessionfile = "~/.apvlvinfo";
#endif

  // Converts the path given to a absolute path.
  // Warning: The string is returned a new allocated buffer, NEED TO BE g_free
  char *absolutepath (const char *path)
  {
    char abpath[PATH_MAX];

    if (g_path_is_absolute (path))
      {
	return g_strdup (path);
      }

    if (*path == '~' && *(path + 1) == PATH_SEP_C)
      {
	const gchar *home;

#ifdef WIN32
	home = g_win32_get_package_installation_directory_of_module (nullptr);
#else
	home = getenv ("HOME");
	if (home == nullptr)
	  {
	    home = g_get_home_dir ();
	  }
#endif

	if (home != nullptr)
	  {
	    g_snprintf (abpath, sizeof abpath, "%s%s", home, ++path);
	  }
	else
	  {
	    debug ("Can't find home directory, use current");
	    g_snprintf (abpath, sizeof abpath, "%s", path + 2);
	  }
      }
    else
      {
	const gchar *pwd;

	pwd = g_get_current_dir ();
	if (pwd != nullptr)
	  {
	    g_snprintf (abpath, sizeof abpath, "%s/%s", pwd, path);
	  }
	else
	  {
	    debug ("Can't find current directory, use current");
	    g_snprintf (abpath, sizeof abpath, "%s", path);
	  }
      }

    return g_strdup (abpath);
  }

  gint strv_find (const gchar **strv, const gchar *key, gboolean cas)
  {
    for (gsize i=0; i<g_strv_length((gchar **) strv); ++ i)
      {
        if (cas && g_ascii_strcasecmp (strv[i], key) == 0)
          {
            return (gint) i;
          }
        else if (cas == FALSE && g_strcmp0 (strv[i], key) == 0)
          {
            return (gint) i;
          }
      }

    return -1;
  }

  gboolean walkdir (const char *name, gboolean (*cb) (const char *, void *),
		    void *usrp)
  {
    GDir *dir = g_dir_open (name, 0, nullptr);
    if (dir == nullptr)
      {
	debug ("Open dir: %s failed", name);
	return FALSE;
      }

    const gchar *token;
    while ((token = g_dir_read_name (dir)) != nullptr)
      {
	gchar *subname = g_strjoin (PATH_SEP_S, name, token, nullptr);
	if (subname == nullptr)
	  {
	    continue;
	  }

	if (g_file_test (subname, G_FILE_TEST_IS_REGULAR) == TRUE)
	  {
	    if (cb (subname, usrp) == FALSE)
	      {
		return FALSE;
	      }
	  }
	else if (g_file_test (subname, G_FILE_TEST_IS_DIR) == TRUE)
	  {
	    if (walkdir (subname, cb, usrp) == FALSE)
	      {
		return FALSE;
	      }
	  }
      }

    g_dir_close (dir);

    return TRUE;
  }

  // Copy a file
  bool filecpy (const char *dst, const char *src)
  {
    gchar *content;
    gchar *s = absolutepath (src);
    gchar *d = absolutepath (dst);
    bool ok = false;

    gboolean ret = g_file_get_contents (s, &content, nullptr, nullptr);
    if (ret == TRUE)
      {
	ret = g_file_set_contents (d, content, -1, nullptr);
	g_free (content);
	ok = ret;
      }

    g_free (s);
    g_free (d);

    return ok;
  }

  bool rmrf (const char *path)
  {
    GDir *dir = g_dir_open (path, 0, nullptr);
    if (dir == nullptr)
      {
	debug ("Open dir: %s failed", path);
	return FALSE;
      }

    const gchar *token;
    while ((token = g_dir_read_name (dir)) != nullptr)
      {
	gchar *subname = g_strjoin (PATH_SEP_S, path, token, nullptr);
	if (subname == nullptr)
	  {
	    continue;
	  }

	if (g_file_test (subname, G_FILE_TEST_IS_REGULAR) == TRUE)
	  {
	    g_unlink (subname);
	  }
	else if (g_file_test (subname, G_FILE_TEST_IS_DIR) == TRUE)
	  {
	    rmrf (subname);
	  }

        g_free (subname);
      }

    g_dir_close (dir);

    g_rmdir (path);

    return TRUE;
  }

  // replace a widget with a new widget
  // return the parent widget
  GtkWidget *replace_widget (GtkWidget * owid, GtkWidget * nwid)
  {
    GtkWidget *parent = gtk_widget_get_parent (owid);
    debug ("parent: %p, owid: %p, nwid: %p", parent, owid, nwid);
    gtk_container_remove (GTK_CONTAINER (parent), owid);
    gtk_container_add (GTK_CONTAINER (parent), nwid);
    gtk_widget_show_all (parent);
    return parent;
  }

  void
  logv (const char *level, const char *file, int line, const char *func,
	const char *ms, ...)
  {
    char p[0x1000], temp[0x100];
    va_list vap;

    g_snprintf (temp, sizeof temp, "[%s] %s: %d: %s(): ",
		level, file, line, func);

    va_start (vap, ms);
    vsnprintf (p, sizeof p, ms, vap);
    va_end (vap);

    cerr << temp << p << endl;
  }

  int apvlv_system (const char *str)
  {
#ifndef WIN32
    int ret;
    pid_t pid;
    int status;

    pid = fork ();

    ret = -1;
    if (pid < 0)
      {
	errp ("Can't fork\n");
      }
    else if (pid == 0)
      {
	gchar **argv;

	while (!isalnum (*str))
	  str++;

	argv = g_strsplit_set (str, " \t", 0);
	if (argv == nullptr)
	  {
	    exit (1);
	  }

	debug ("Exec path: (%s) argument [%d]\n", argv[0],
	       g_strv_length (argv));
	ret = execvp (argv[0], argv);
	g_strfreev (argv);
	errp ("Exec error\n");
      }
    else
      {
	ret = wait4 (pid, &status, 0, nullptr);
      }

    return ret;
#else
    return WinExec (str, SW_NORMAL);
#endif
  }
}

// Local Variables:
// mode: c++
// End:
