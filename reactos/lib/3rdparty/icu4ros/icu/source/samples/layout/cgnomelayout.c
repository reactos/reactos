
/*
 ****************************************************************************** *
 *
 *   Copyright (C) 1999-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 ****************************************************************************** *
 */

#include <gnome.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "pflow.h"

#include "gnomeglue.h"

#include "arraymem.h"

struct Context
{
    long width;
    long height;
    pf_flow *paragraph;
};

typedef struct Context Context;

static FT_Library engine;
static gs_guiSupport *guiSupport;
static fm_fontMap *fontMap;
static le_font *font;

static GSList *appList = NULL;

GtkWidget *newSample(const gchar *fileName);
void       closeSample(GtkWidget *sample);

static void showabout(GtkWidget *widget, gpointer data)
{
    GtkWidget *aboutBox;
    const gchar *documentedBy[] = {NULL};
    const gchar *writtenBy[] = {
        "Eric Mader",
        NULL
    };

    aboutBox = gnome_about_new("Gnome Layout Sample",
                               "0.1",
                               "Copyright (C) 1998-2006 By International Business Machines Corporation and others. All Rights Reserved.",
                               "A simple demo of the ICU LayoutEngine.",
                               writtenBy,
                               documentedBy,
                               "",
                               NULL);

    gtk_widget_show(aboutBox);
}

#if 0
static void notimpl(GtkObject *object, gpointer data)
{
    gnome_ok_dialog("Not implemented...");
}
#endif

static gchar *prettyTitle(const gchar *path)
{
  const gchar *name  = g_basename(path);
  gchar *title = g_strconcat("Gnome Layout Sample - ", name, NULL);

  return title;
}

static void openOK(GtkObject *object, gpointer data)
{
  GtkFileSelection *fileselection = GTK_FILE_SELECTION(data);
  GtkWidget *app = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(fileselection), "app"));
  Context *context = (Context *) gtk_object_get_data(GTK_OBJECT(app), "context");
  gchar *fileName  = g_strdup(gtk_file_selection_get_filename(fileselection));
  pf_flow *newPara;

  gtk_widget_destroy(GTK_WIDGET(fileselection));

  newPara = pf_factory(fileName, font, guiSupport);

  if (newPara != NULL) {
    gchar *title = prettyTitle(fileName);
    GtkWidget *area = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(app), "area"));

    if (context->paragraph != NULL) {
      pf_close(context->paragraph);
    }

    context->paragraph = newPara;
    gtk_window_set_title(GTK_WINDOW(app), title);

    gtk_widget_hide(area);
    pf_breakLines(context->paragraph, context->width, context->height);
    gtk_widget_show_all(area);

    g_free(title);
  }

  g_free(fileName);
}

static void openfile(GtkObject *object, gpointer data)
{
  GtkWidget *app = GTK_WIDGET(data);
  GtkWidget *fileselection;
  GtkWidget *okButton;
  GtkWidget *cancelButton;

  fileselection =
    gtk_file_selection_new("Open File");

  gtk_object_set_data(GTK_OBJECT(fileselection), "app", app);

  okButton =
    GTK_FILE_SELECTION(fileselection)->ok_button;

  cancelButton =
    GTK_FILE_SELECTION(fileselection)->cancel_button;

  gtk_signal_connect(GTK_OBJECT(fileselection), "destroy",
             GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  gtk_signal_connect(GTK_OBJECT(okButton), "clicked",
             GTK_SIGNAL_FUNC(openOK), fileselection);

  gtk_signal_connect_object(GTK_OBJECT(cancelButton), "clicked",
             GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(fileselection));

  gtk_window_set_modal(GTK_WINDOW(fileselection), TRUE);
  gtk_widget_show(fileselection);
  gtk_main();
}

static void newapp(GtkObject *object, gpointer data)
{
  GtkWidget *app = newSample("Sample.txt");

  gtk_widget_show_all(app);
}

static void closeapp(GtkWidget *widget, gpointer data)
{
  GtkWidget *app = GTK_WIDGET(data);

  closeSample(app);
}

static void shutdown(GtkObject *object, gpointer data)
{
    gtk_main_quit();
}

GnomeUIInfo fileMenu[] =
{
  GNOMEUIINFO_MENU_NEW_ITEM((gchar *) "_New Sample",
                (gchar *) "Create a new Gnome Layout Sample",
                newapp, NULL),

  GNOMEUIINFO_MENU_OPEN_ITEM(openfile, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_CLOSE_ITEM(closeapp, NULL),
  GNOMEUIINFO_MENU_EXIT_ITEM(shutdown, NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo helpMenu[] =
{
    /* GNOMEUIINFO_HELP("gnomelayout"), */
    GNOMEUIINFO_MENU_ABOUT_ITEM(showabout, NULL),
    GNOMEUIINFO_END
};

GnomeUIInfo mainMenu[] =
{
    GNOMEUIINFO_SUBTREE(N_((gchar *) "File"), fileMenu),
    GNOMEUIINFO_SUBTREE(N_((gchar *) "Help"), helpMenu),
    GNOMEUIINFO_END
};

static gint eventDelete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  closeSample(widget);

  /* indicate that closeapp  already destroyed the window */
  return TRUE;
}

static gint eventConfigure(GtkWidget *widget, GdkEventConfigure *event, Context *context)
{
  if (context->paragraph != NULL) {
    context->width  = event->width;
    context->height = event->height;

    if (context->width > 0 && context->height > 0) {
        pf_breakLines(context->paragraph, context->width, context->height);
    }
  }

  return TRUE;
}

static gint eventExpose(GtkWidget *widget, GdkEvent *event, Context *context)
{
  if (context->paragraph != NULL) {
    gint maxLines = pf_getLineCount(context->paragraph) - 1;
    gint firstLine = 0, lastLine = context->height / pf_getLineHeight(context->paragraph);
    rs_surface *surface = rs_gnomeRenderingSurfaceOpen(widget);

    pf_draw(context->paragraph, surface, firstLine, (maxLines < lastLine)? maxLines : lastLine);
    
    rs_gnomeRenderingSurfaceClose(surface);
  }

  return TRUE;
}

GtkWidget *newSample(const gchar *fileName)
{
  Context *context = NEW_ARRAY(Context, 1);
  gchar *title;
  GtkWidget *app;
  GtkWidget *area;
  GtkStyle *style;
  int i;

  context->width  = 600;
  context->height = 400;
  context->paragraph = pf_factory(fileName, font, guiSupport);

  title = prettyTitle(fileName);
  app = gnome_app_new("gnomeLayout", title);

  gtk_object_set_data(GTK_OBJECT(app), "context", context);

  gtk_window_set_default_size(GTK_WINDOW(app), 600 - 24, 400);

  gnome_app_create_menus_with_data(GNOME_APP(app), mainMenu, app);

  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
             GTK_SIGNAL_FUNC(eventDelete), NULL);

  area = gtk_drawing_area_new();
  gtk_object_set_data(GTK_OBJECT(app), "area", area);

  style = gtk_style_copy(gtk_widget_get_style(area));

  for (i = 0; i < 5; i += 1) {
    style->fg[i] = style->white;
  }
    
  gtk_widget_set_style(area, style);

  gnome_app_set_contents(GNOME_APP(app), area);

  gtk_signal_connect(GTK_OBJECT(area),
             "expose_event",
             GTK_SIGNAL_FUNC(eventExpose),
             context);

  gtk_signal_connect(GTK_OBJECT(area),
             "configure_event",
             GTK_SIGNAL_FUNC(eventConfigure),
             context);

  appList = g_slist_prepend(appList, app);

  g_free(title);

  return app;
}

void closeSample(GtkWidget *app)
{
  Context *context = (Context *) gtk_object_get_data(GTK_OBJECT(app), "context");

  if (context->paragraph != NULL) {
    pf_close(context->paragraph);
  }

  DELETE_ARRAY(context);

  appList = g_slist_remove(appList, app);

  gtk_widget_destroy(app);

  if (appList == NULL) {
    gtk_main_quit();
  }
}

int main (int argc, char *argv[])
{
    LEErrorCode   fontStatus = LE_NO_ERROR;
    poptContext   ptctx;
    GtkWidget    *app;
    const char   *defaultArgs[] = {"Sample.txt", NULL};
    const char  **args;
    int i;

    FT_Init_FreeType(&engine);

    gnome_init_with_popt_table("gnomelayout", "0.1", argc, argv, NULL, 0, &ptctx);

    guiSupport = gs_gnomeGuiSupportOpen();
    fontMap    = fm_gnomeFontMapOpen(engine, "FontMap.Gnome", 24, guiSupport, &fontStatus);
    font       = le_scriptCompositeFontOpen(fontMap);

    if (LE_FAILURE(fontStatus)) {
        FT_Done_FreeType(engine);
        return 1;
    }

    args = poptGetArgs(ptctx);
    
    if (args == NULL) {
        args = defaultArgs;
    }

    for (i = 0; args[i] != NULL; i += 1) {
       app = newSample(args[i]);
           
       gtk_widget_show_all(app);
    }
    
    poptFreeContext(ptctx);
    
    gtk_main();

    le_fontClose(font);
    gs_gnomeGuiSupportClose(guiSupport);

    FT_Done_FreeType(engine);

    exit(0);
}
