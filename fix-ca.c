/*
 * fix-ca.c -  Gimp Plug-In Fix Chromatic Aberration
 * Copyright (c) 2006, 2007 Kriang Lerdsuwanakij - (original author)
 * Copyright (c) 2023, 2024 Jose Da Silva (updates and improvements)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#if __has_include("fix-ca-config.h")
#include "fix-ca-config.h"
#else
#define FIX_CA_VERSION "fix-CA local"
#endif

#include <string.h>
#include <math.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#ifdef GDK_WINDOWING_QUARTZ
#import <Cocoa/Cocoa.h>
#elif defined (G_OS_WIN32)
#include <windows.h>
#endif

#ifdef xxHAVE_GETTEXT
#include <libintl.h>
#define _(String) gettext (String)
#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif
#else
/* No i18n for now */
#define _(x)	x
#define N_(x)	x
#endif

#ifdef DEBUG_TIME
# include <sys/time.h>
# include <stdio.h>
#endif

#define PLUG_IN_PROC        "plug-in-fix-ca"
#define PLUG_IN_BINARY      "fix-ga"
#define PLUG_IN_ROLE        "gimp-fix-ca"
//-------#define SPIN_BUTTON_WIDTH    8
//-------#define COLOR_BUTTON_WIDTH  55

#define PROCEDURE_NAME	"Fix-CA"
#define DATA_KEY_VALS	"fix_ca"

/* Size controls in Fix CA dialog box */
#define SCALE_WIDTH	150
#define ENTRY_WIDTH	4

/* For row buffer management */
#define SOURCE_ROWS	120
#define INPUT_MAX	SOURCE_ROWS/4
#define ROW_INVALID	-100
#define ITER_INITIAL	-100

typedef struct _FixCa	FixCa;
typedef struct _FixCaClass FixCaClass;

struct _FixCa {
  GimpPlugIn parent_instance;
};

struct _FixCaClass {
  GimpPlugInClass parent_class;
};

/* Storage type */
typedef struct {
  gdouble  blue;
  gdouble  red;
  gdouble  lens_x;
  gdouble  lens_y;
  gboolean update_preview;
  GimpInterpolationType	interpolation;
  gdouble  saturation;
  gdouble  x_blue;
  gdouble  x_red;
  gdouble  y_blue;
  gdouble  y_red;
  /* private */
  GimpDrawable *drawable;
} FixCaParams;

/* Global default */
static const FixCaParams fix_ca_params_default = {
  0.0,  /* blue */
  0.0,  /* red  */
  -1.0, /* lens_x */
  -1.0, /* lens_y */
  TRUE, /* update preview */
  GIMP_INTERPOLATION_LINEAR, /* do linear interpolation */
  0.0,  /* saturation */
  0.0,  /* x_blue */
  0.0,  /* x_red  */
  0.0,  /* y_blue */
  0.0,  /* y_red  */
  NULL
};

/* Declare local functions. */
#define FIXCA_TYPE  (fixca_get_type())
#define FIXCA (obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FIXCA_TYPE, FixCa))

GType                  fixca_get_type        (void) G_GNUC_CONST;

static GList	      *fixca_query_procedures(GimpPlugIn	   *plug_in);
static GimpProcedure  *fixca_create_procedure(GimpPlugIn	   *plug_in,
                                              const gchar	   *name);

static GimpValueArray *fixca_run	     (GimpProcedure	   *procedure,
					      GimpRunMode	    run_mode,
					      GimpImage		   *image,
					      gint		    n_drawables,
					      GimpDrawable	  **drawables,
					      const GimpValueArray *args,
					      gpointer		    run_data);

static int	       fix_ca		     (GimpDrawable	   *drawable,
					      FixCaParams	   *params);

gboolean	       fix_ca_dialog	     (GimpDrawable	   *drawable,
					      FixCaParams	   *params);

static void	       fix_ca_region	     (guchar		   *srcPTR,
					      guchar		   *dstPTR,
					      gint		    orig_width,
					      gint		    orig_height,
					      gint		    bytes,
					      gint		    bpc,
					      FixCaParams	   *params,
					      gint		    x1,
					      gint		    x2,
					      gint		    y1,
					      gint		    y2,
					      gboolean		    show_progress);

static void	       fix_ca_help	     (const gchar	   *help_id,
					      gpointer		    help_data);

void		       preview_update	     (GimpPreview	   *preview,
					      FixCaParams	   *params);

G_DEFINE_TYPE(FixCa, fixca, GIMP_TYPE_PLUG_IN)

GIMP_MAIN(FIXCA_TYPE)
//DEFINE_STD_SET_I18N; ***don't use since this is 3rd-party locale external to gimp

static void fixca_class_init(FixCaClass *klass) {
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(klass);

  plug_in_class->query_procedures = fixca_query_procedures;
  plug_in_class->create_procedure = fixca_create_procedure;

  //plug_in_class->set_i18n         = STD_SET_I18N;
}

static void fixca_init(FixCa *fixca) {
}

static GList *fixca_query_procedures(GimpPlugIn *plug_in) {
  return g_list_append(NULL, g_strdup(PLUG_IN_PROC));
}

static GimpProcedure *fixca_create_procedure(GimpPlugIn *plug_in,
					     const gchar *name) {
  GimpProcedure *procedure = NULL;

  if (!strcmp(name, PLUG_IN_PROC)) {
    procedure = gimp_image_procedure_new(plug_in, name,
					 GIMP_PDB_PROC_TYPE_PLUGIN,
					 fixca_run, NULL, NULL);

    gimp_procedure_set_image_types(procedure, "*");
    gimp_procedure_set_sensitivity_mask(procedure,
					GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

#ifdef HAVE_GETTEXT
    /* Initialize i18n support */
    //setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, gimp_locale_directory());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
    //textdomain(GETTEXT_PACKAGE);
#endif

    gimp_procedure_set_menu_label(procedure, _("Chromatic Aberration"));
    gimp_procedure_add_menu_path(procedure, _("<Image>/Filters/Colors"));
    gimp_procedure_set_documentation(procedure,
				     _("Fix chromatic aberration"),
				     _("Fix chromatic aberration caused by "
				     "imperfect lens.  It works by shifting "
				     "red and blue components of image pixels "
				     "in the specified amounts."),
				     PLUG_IN_PROC);
    gimp_procedure_set_attribution(procedure,
				   "Kriang Lerdsuwanakij",
				   "Kriang Lerdsuwanakij 2006,2007, Jose Da Silva 2022..2024",
				   "2006 - 2024");

    GIMP_PROC_ARG_DOUBLE(procedure, "blue",
			 _("_Blue"), _("Blue amount (lateral)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE(procedure, "red",
			 _("_Red"), _("Red amount (lateral)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT(procedure, "lens_x",
		      _("lens_X"), _("Lens center X (lateral)"),
		      -1, GIMP_MAX_IMAGE_SIZE, -1, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT(procedure, "lens_y",
		      _("lens_Y"), _("Lens center Y (lateral)"),
		      -1, GIMP_MAX_IMAGE_SIZE, -1, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT(procedure, "interpolation",
		      _("_interpolation"),
		      _("Interpolation 0=None/1=Linear/2=Cubic"),
		      0, 2, 1, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE(procedure, "x_blue",
			 _("X Blue"), _("Blue amount, X axis (directional)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE(procedure, "x_red",
			 _("X Red"), _("Red amount, X axis (directional)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE(procedure, "y_blue",
			 _("Y Blue"), _("Blue amount, Y axis (directional)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE(procedure, "y_red",
			 _("Y Red"), _("Red amount, Y axis (directional)"),
			 -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
  }

  return procedure;
}

static GimpValueArray *fixca_run(GimpProcedure	      *procedure,
				 GimpRunMode	       run_mode,
				 GimpImage	      *image,
				 gint		       n_drawables,
				 GimpDrawable	     **drawables,
				 const GimpValueArray *args,
				 gpointer	       run_data) {
  GimpDrawable      *drawable;
  FixCaParams        fix_ca_params;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error = NULL;

#ifdef HAVE_GETTEXT
  /* Initialize i18n support */
  //setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, gimp_locale_directory());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
  //textdomain(GETTEXT_PACKAGE);
#endif

  if (n_drawables != 1) {
    g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
		_("Procedure '%s' only works with one drawable."),
		PLUG_IN_PROC);

    return gimp_procedure_new_return_values(procedure,
					    GIMP_PDB_CALLING_ERROR,
					    error);
  } else {
    fix_ca_params.drawable = drawable = drawables[0];
  }
  if (!gimp_drawable_is_rgb(drawable)) {
    g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
		_("Procedure '%s' only works with RGB or RGBA."),
		PLUG_IN_PROC);

    return gimp_procedure_new_return_values(procedure,
					    GIMP_PDB_CALLING_ERROR,
					    error);
  }

  if (run_mode == GIMP_RUN_NONINTERACTIVE) {
    fix_ca_params.blue = GIMP_VALUES_GET_DOUBLE(args, 0);
    fix_ca_params.red  = GIMP_VALUES_GET_DOUBLE(args, 1);
//    if (nparams < 3)
      fix_ca_params.lens_x = fix_ca_params_default.lens_y;
//    else
      fix_ca_params.lens_x = GIMP_VALUES_GET_INT(args, 2);
//    if (nparams < 4)
      fix_ca_params.lens_y = fix_ca_params_default.lens_y;
//    else
      fix_ca_params.lens_y = GIMP_VALUES_GET_INT(args, 3);
//    if (nparams < 5)
      fix_ca_params.interpolation = GIMP_INTERPOLATION_NONE;
//    else
      fix_ca_params.interpolation = GIMP_VALUES_GET_INT(args, 4);
//    if (nparams < 6)
      fix_ca_params.x_blue = fix_ca_params_default.x_blue;
//    else
      fix_ca_params.x_blue = GIMP_VALUES_GET_DOUBLE(args, 5);
//    if (nparams < 7)
      fix_ca_params.x_red  = fix_ca_params_default.x_red;
//    else
      fix_ca_params.x_red  = GIMP_VALUES_GET_DOUBLE(args, 6);
//    if (nparams < 8)
      fix_ca_params.y_blue = fix_ca_params_default.y_blue;
//    else
      fix_ca_params.y_blue = GIMP_VALUES_GET_DOUBLE(args, 7);
//    if (nparams < 9)
      fix_ca_params.y_red  = fix_ca_params_default.y_red;
//    else
      fix_ca_params.y_red  = GIMP_VALUES_GET_DOUBLE(args, 8);

    if (fix_ca_params.blue < -INPUT_MAX || \
	fix_ca_params.blue >  INPUT_MAX || \
	fix_ca_params.red  < -INPUT_MAX || \
	fix_ca_params.red  >  INPUT_MAX || \
	fix_ca_params.interpolation < 0 || \
	fix_ca_params.interpolation > 2 || \
	fix_ca_params.x_blue < -INPUT_MAX || \
	fix_ca_params.x_blue >  INPUT_MAX || \
	fix_ca_params.x_red  < -INPUT_MAX || \
	fix_ca_params.x_red  >  INPUT_MAX || \
	fix_ca_params.y_blue < -INPUT_MAX || \
	fix_ca_params.y_blue >  INPUT_MAX || \
	fix_ca_params.y_red  < -INPUT_MAX || \
	fix_ca_params.y_red  >  INPUT_MAX) {
      g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
		  _("Parameter out of range!"));
      return gimp_procedure_new_return_values(procedure,
					      GIMP_PDB_CALLING_ERROR,
					      error);
    }
  }

  // NOTE: removed gimp_set_data() and gimp_get_data() in gimp3
  //if (run_mode == GIMP_RUN_WITH_LAST_VALS) {
  //  gimp_get_data(DATA_KEY_VALS, &fix_ca_params);
  //  fix_ca_params.drawable = drawable;
  //}

  /* In interactive mode, display a dialog to advertise the exercise. */
  if (run_mode == GIMP_RUN_INTERACTIVE) {
    if (gimp_display_name() == NULL) {
      /* someone wants to run this plugin using `gimp --no-interface` */
      g_set_error(&error, GIMP_PLUG_IN_ERROR, 0,
		  _("No interactive GUI interface found!"));
      return gimp_procedure_new_return_values(procedure,
					      GIMP_PDB_CALLING_ERROR,
					      error);
    }

    if (!fix_ca_dialog(drawable, &fix_ca_params)) {
      return gimp_procedure_new_return_values(procedure,
					      GIMP_PDB_CANCEL,
					      NULL);
    }
  }

  if (status == GIMP_PDB_SUCCESS && fix_ca(drawable, &fix_ca_params)) {
    status = GIMP_PDB_CALLING_ERROR;
  } else {
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
      gimp_displays_flush ();

  // NOTE: removed gimp_set_data() and gimp_get_data() in gimp3
  // if (run_mode == GIMP_RUN_INTERACTIVE)
  //  gimp_set_data(DATA_KEY_VALS, &fix_ca_params, sizeof(fix_ca_params));

    //---------gimp_drawable_detach(drawable);
  }

  return gimp_procedure_new_return_values(procedure, status, NULL);
}

static int color_size(const Babl *format) {
  int         bpc = babl_format_get_bytes_per_pixel(format);
  const char *str = babl_get_name(format);
#ifdef DEBUG_TIME
  printf("color_size(), format=<%s>, bytes per pixel=%d\n", str, bpc);
#endif
  if (strstr(str, "double") != NULL)
    return -8; /* IEEE 754 double precision */
  if (strstr(str, "float") != NULL)
    return -4; /* IEEE 754 single precision */
  //if (strstr(str, "half") != NULL)
  //  return -2; /* IEEE 754 half precision */
  if (strstr(str, "u15") != NULL)
    return -99; /* TODO for another day */
  if (strstr(str, " u") == NULL)
    return -99; /* not unsigned integer size */
  if (bpc > 32)
    return -99; /* unknown size */
  if (bpc >= 24)
    return 8;
  if (bpc >= 12)
    return 4;
  if (bpc >= 6)
    return 2;
  if (bpc >= 3)
    return 1;

  /* probably RGB in 1 byte, TODO for another day */
  return -99;
}

static gdouble get_pixel(guchar *ptr, gint bpc) {
  /* Returned value is in the range of [0.0..1.0]. */
  gdouble ret = 0.0;
  if (bpc == 1) {
    ret += *ptr;
    ret /= 255;
  } else if (bpc == 2) {
    uint16_t *p = (uint16_t *)(ptr);
    ret += *p;
    ret /= 65535;
   } else if (bpc == 4) {
    uint32_t *p = (uint32_t *)(ptr);
    ret += *p;
    ret /= 4294967295;
  } else if (bpc == 8) {
    uint64_t *p = (uint64_t *)(ptr);
    long double lret = 0.0;
    lret += *p;
    lret /= 18446744073709551615L;
    ret = lret;
  } else if (bpc == -8) {
    double *p = (double *)(ptr);
    ret += *p;
  } else if (bpc == -4) {
    float *p = (float *)(ptr);
    ret += *p;
//} else if (bpc == -2) {
//  half *p = ptr;
//  ret += *p;
  }
  return ret;
}

static void set_pixel(guchar *dest, gdouble d, gint bpc) {
  /* input value is in the range of [0.0..1.0]. */
  if (bpc == 1) {
    *dest = round(d * 255);
  }  else if (bpc == 2) {
    uint16_t *p = (uint16_t *)(dest);
    *p = round(d * 65535);
  } else if (bpc == 4) {
    uint32_t *p = (uint32_t *)(dest);
    *p = round(d * 4294967295);
  } else if (bpc == 8) {
    uint64_t *p = (uint64_t *)(dest);
    *p = roundl(d * 18446744073709551615L);
  } else if (bpc == -8) {
    double *p = (double *)(dest);
    *p = d;
  } else if (bpc == -4) {
    float *p = (float *)(dest);
    *p = (float)(d);
//} else if (bpc == -2) {
//  half *p = (half *)(dest);
//  *p = d;
  }
  return;
}

static int round_nearest(gdouble d) {
  if (d >= 0) {
    if (d > INT_MAX)
      return INT_MAX;
    else
      return (int)(d + 0.5);
  } else {
    if (d < INT_MIN)
      return INT_MIN;
    else
      return -((int)(0.5 - d));
  }
}

static int absolute(gint i) {
  if (i >= 0)
    return i;
  else
    return -i;
}

static int fix_ca(GimpDrawable *drawable, FixCaParams *params) {
  GeglBuffer *srcBuf, *destBuf;
  guchar     *srcImg, *destImg;
  const Babl *format;
  gint       x, y, width, height, xImg, yImg, bppImg, bpcImg;

  /* get dimensions */
  if (!(gimp_drawable_mask_intersect(drawable, &x, &y, &width, &height)))
    return -1;

#ifdef DEBUG_TIME
  double	sec;
  struct timeval tv1, tv2;
  gettimeofday(&tv1, NULL);

  printf("Start fix_ca(), x=%d y=%d width=%d height=%d\n", x, y, width, height);
#endif

  format = gimp_drawable_get_format(drawable);
  bppImg = gimp_drawable_get_bpp(drawable);
  bpcImg = color_size(format);
  if (bpcImg <= -99) {
    g_message(_("Invalid color type!"));
    return -1;
  }

  //gegl_init (NULL, NULL);

  /* fetch pixel regions and setup shadow buffer */
  srcBuf  = gimp_drawable_get_buffer(drawable);
  destBuf = gimp_drawable_get_shadow_buffer(drawable);

  xImg = gimp_drawable_get_width(drawable);
  yImg = gimp_drawable_get_height(drawable);
  srcImg  = g_new(guchar, xImg * yImg * bppImg);
  destImg = g_new(guchar, xImg * yImg * bppImg);

  gegl_buffer_get(srcBuf, GEGL_RECTANGLE(x, y, width, height), 1.0,
		  format, srcImg, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* adjust pixel regions from srcImg to destImg, according to params */
  fix_ca_region(srcImg, destImg, xImg, yImg, bppImg, bpcImg, params,
		x, (x+width), y, (y+height), TRUE);

  gegl_buffer_set(destBuf, GEGL_RECTANGLE(x, y, width, height), 0,
		  format, destImg, GEGL_AUTO_ROWSTRIDE);

  g_free(destImg);
  g_free(srcImg);
  g_object_unref(destBuf);
  g_object_unref(srcBuf);

  gimp_drawable_merge_shadow(drawable, TRUE);
  gimp_drawable_update(drawable, x, y, width, height);

  //gegl_exit ();

#ifdef DEBUG_TIME
  gettimeofday(&tv2, NULL);

  sec = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
  printf("End fix-ca(), Elapsed time: %.2f\n", sec);
#endif
  return 0;
}

void preview_update(GimpPreview *preview, FixCaParams *params) {
  gint	      b, i, j, x, y, width, height, xImg, yImg, bppImg, bpcImg, size;
  GeglBuffer *srcBuf;
  guchar     *srcImg, *destImg, *prevImg;
  const Babl *format;
  gdouble     d;

  gimp_preview_get_position(preview, &x, &y);
  gimp_preview_get_size(preview, &width, &height);

  format = gimp_drawable_get_format(params->drawable);
  bppImg = gimp_drawable_get_bpp(params->drawable);
  bpcImg = color_size(format);
#ifdef DEBUG_TIME
  printf("preview_update(), bppImg=%d, bpcImg=%d, x=%d y=%d w=%d h=%d\n",
	 bppImg, bpcImg, x, y, width, height);
#endif
  if (bpcImg <= -99)
    return;

  xImg = gimp_drawable_get_width(params->drawable);
  yImg = gimp_drawable_get_height(params->drawable);
  size = xImg * yImg * bppImg;
  srcImg  = g_new(guchar, size);
  destImg = g_new(guchar, size);
  prevImg = g_new(guchar, width * height * bppImg);

  srcBuf  = gimp_drawable_get_buffer(params->drawable);
  gegl_buffer_get(srcBuf, GEGL_RECTANGLE(0, 0, xImg, yImg), 1.0,
		  format, srcImg, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  fix_ca_region(srcImg, destImg, xImg, yImg, bppImg, bpcImg, params,
		0, xImg, y, (y + height), FALSE);

  b = absolute(bpcImg);
  for (i = 0; i < height; i++) {
    if (b == 1) {
      memcpy(&prevImg[width * i * bppImg],
	     &destImg[(xImg * (y + i) + x) * bppImg],
	     width * bppImg);
    } else {
      for (j = 0; j < width*bppImg/b; j++) {
	d = get_pixel(&destImg[(xImg*(y+i)+x)*bppImg+j*b], bpcImg);
	set_pixel(&prevImg[width*i*bppImg/b+j], d, 1);
      }
    }
  }

  gimp_preview_draw_buffer(preview, prevImg, width * bppImg/b);

  g_object_unref(srcBuf);
  g_free(prevImg);
  g_free(destImg);
  g_free(srcImg);
}

gboolean fix_ca_dialog(GimpDrawable *drawable, FixCaParams *params) {
  gchar     *title;
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *combo;
  GtkWidget *preview;
  GtkWidget *grid;
  GtkWidget *frame;
//  GtkObject *adj;
  GtkWidget *adj;
  gboolean   run;
  gint       xImg, yImg;

//  gimp_ui_init("fix_ca");
  title = g_strdup_printf(_("Chromatic Aberration"));
  dialog = gimp_dialog_new(title, "fix_ca",
			   NULL, (GtkDialogFlags)(0),
			   fix_ca_help, "plug-in-fix-ca",
//                     _("_Reset"),  RESPONSE_RESET,
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"),     GTK_RESPONSE_OK,
			   NULL);
  g_free(title);

  main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  //gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     main_vbox, TRUE, TRUE, 0);
  gtk_widget_show(main_vbox);

  preview = gimp_drawable_preview_new_from_drawable(drawable);
  xImg = gimp_drawable_get_width(drawable);
  yImg = gimp_drawable_get_height(drawable);
  if (params->lens_x < 0 || params->lens_x >= xImg)
    params->lens_x = round(xImg/2);
  if (params->lens_y < 0 || params->lens_y >= yImg)
    params->lens_y = round(yImg/2);

  gtk_box_pack_start(GTK_BOX(main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show(preview);

  g_signal_connect(preview, "invalidated",
		   G_CALLBACK(preview_update),
		   params);

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_box_pack_start(GTK_BOX(main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show(grid);

  adj = gimp_scale_entry_new(_("Preview _saturation:"), params->saturation, -100.0, 100.0, 0);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 1, 10);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->saturation));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  combo = gimp_int_combo_box_new(_("None (Fastest)"), GIMP_INTERPOLATION_NONE,
				 _("Linear"),	      GIMP_INTERPOLATION_LINEAR,
				 _("Cubic (Best)"),   GIMP_INTERPOLATION_CUBIC,
				 NULL);

  gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(combo),
			     params->interpolation,
			     G_CALLBACK(gimp_int_combo_box_get_active),
			     &params->interpolation, NULL);

  gimp_grid_attach_aligned(GTK_GRID(grid), 0, 1, _("_Interpolation:"),
                           0.0, 0.5, combo, 2);
  g_signal_connect_swapped(combo, "changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  frame = gimp_frame_new(_("Lateral"));
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_container_add(GTK_CONTAINER(frame), grid);
  gtk_widget_show(grid);

  adj = gimp_scale_entry_new(_("_Blue:"), params->blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->blue));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  adj = gimp_scale_entry_new(_("_Red:"), params->red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->red));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  adj = gimp_scale_entry_new(_("Lens_X:"), params->lens_x, -1, xImg-1, 0);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 10, 100);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->lens_x));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  adj = gimp_scale_entry_new(_("Lens_Y:"), params->lens_y, 0, yImg-1, 0);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 10, 100);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->lens_y));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  frame = gimp_frame_new(_("Directional, X axis"));
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_container_add(GTK_CONTAINER(frame), grid);
  gtk_widget_show(grid);

  adj = gimp_scale_entry_new(_("Blue:"), params->x_blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->x_blue));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  adj = gimp_scale_entry_new(_("Red:"), params->x_red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->x_red));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  frame = gimp_frame_new(_("Directional, Y axis"));
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_container_add(GTK_CONTAINER(frame), grid);
  gtk_widget_show(grid);

  adj = gimp_scale_entry_new(_("Blue:"), params->y_blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->y_blue));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK (gimp_preview_invalidate),
			   preview);

  adj = gimp_scale_entry_new(_("Red:"), params->y_red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments(GIMP_LABEL_SPIN(adj), 0.1, 0.5);

  g_object_set_data(G_OBJECT(adj), "config",  preview);
  g_signal_connect(adj, "value_changed",
		   G_CALLBACK(gimp_double_adjustment_update),
		   &(params->y_red));
  g_signal_connect_swapped(adj, "value_changed",
			   G_CALLBACK(gimp_preview_invalidate),
			   preview);

  gtk_widget_show(dialog);

  run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy(dialog);

  return run;
}

static int scale(gint i, gint center, gint size, gdouble scale_val, gdouble shift_val) {
  gdouble d = (i - center) * scale_val + center - shift_val;
  gint    j = round_nearest (d);
  if (j <= 0)
    return 0;
  else if (j < size)
    return j;
  else
    return size-1;
}

static double scale_d(gint i, gint center, gint size, gdouble scale_val, gdouble shift_val) {
  gdouble d = (i - center) * scale_val + center - shift_val;
  if (d <= 0.0)
    return 0.0;
  else if (d >= size-1)
    return size-1;
  else
    return d;
}

static guchar *load_data(gint fullWidth, gint bpp, guchar *srcPTR,
			 guchar *src[SOURCE_ROWS], gint src_row[SOURCE_ROWS],
			 gint src_iter[SOURCE_ROWS], gint band_adj,
			 gint band_left, gint band_right, gint y, gint iter) {
  gint i, l, x, diff, diff_max = -1, row_best = -1;
  int  iter_oldest;
  for (i = 0; i < SOURCE_ROWS; ++i) {
    if (src_row[i] == y) {
      src_iter[i] = iter; /* Make sure to keep this row during this iteration */
      return src[i];
    }
  }

  /* Find a row to replace */
  iter_oldest = INT_MAX; /* Largest possible */
  for (i = 0; i < SOURCE_ROWS; ++i) {
    if (src_iter[i] < iter_oldest) {
      iter_oldest = src_iter[i];
      diff_max = absolute (y - src_row[i]);
      row_best = i;
    } else if (src_iter[i] == iter_oldest) {
      diff = absolute (y - src_row[i]);
      if (diff > diff_max) {
	diff_max = diff;
	row_best = i;
      }
    }
  }

  x = ((fullWidth * y) + band_left) * bpp;
  i = band_adj * bpp;
  l = i + (band_right-band_left+1) * bpp;
  memcpy(&src[row_best][i], &srcPTR[x + i], l - i);
  src_row[row_best] = y;
  src_iter[row_best] = iter;
  return src[row_best];
}

static void set_data(guchar *dstPTR, guchar *dest, gint bpp,
		     gint fullWidth, gint xstart, gint yrow, gint width) {
  gint l, x;
  x = ((fullWidth * yrow) + xstart) * bpp;
  l = width * bpp;
  memcpy(&dstPTR[x], dest, l);
}

static gdouble clip_d(gdouble d) {
  if (d <= 0.0)
    return 0.0;
  if (d >= 1.0)
    return 1.0;
  return d;
}

static void bilinear(guchar *dest,
		     guchar *yrow0, guchar *yrow1, gint x0, gint x1,
		     gint bpp, gint bpc, gdouble dx, gdouble dy) {
  gdouble d, x0y0, x1y0, x0y1, x1y1;
  x0y0 = get_pixel(&yrow0[x0*bpp], bpc);
  x1y0 = get_pixel(&yrow0[x1*bpp], bpc);
  x0y1 = get_pixel(&yrow1[x0*bpp], bpc);
  x1y1 = get_pixel(&yrow1[x1*bpp], bpc);
  d = (1-dy) * (x0y0 + dx * (x1y0-x0y0))
       + dy  * (x0y1 + dx * (x1y1-x0y1));
  set_pixel(dest, clip_d(d), bpc);
}

static gdouble cubicY(guchar *yrow, gint bpp, gint bpc, gdouble dx,
		      gint m1, gint x0, gint p1, gint p2) {
  /* Catmull-Rom from Gimp gimpdrawable-transform.c */
  gdouble d, xm1, x, xp1, xp2;
  xm1 = get_pixel(&yrow[m1*bpp], bpc);
  x   = get_pixel(&yrow[x0*bpp], bpc);
  xp1 = get_pixel(&yrow[p1*bpp], bpc);
  xp2 = get_pixel(&yrow[p2*bpp], bpc);
  d = ((( ( - xm1 + 3 * x - 3 * xp1 + xp2 ) * dx +
        ( 2 * xm1 - 5 * x + 4 * xp1 - xp2 ) ) * dx +
	  ( - xm1 + xp1 ) ) * dx + (x + x) ) / 2.0;
  return d;
}

static void cubicX(guchar *dest, gint bpp, gint bpc, gdouble dy,
		   gdouble ym1, gdouble y, gdouble yp1, gdouble yp2) {
  /* Catmull-Rom from Gimp gimpdrawable-transform.c */
  gdouble d;
  d = ((( ( - ym1 + 3 * y - 3 * yp1 + yp2 ) * dy +
        ( 2 * ym1 - 5 * y + 4 * yp1 - yp2 ) ) * dy +
	  ( - ym1 + yp1 ) ) * dy + (y + y) ) / 2.0;
  set_pixel(dest, clip_d(d), bpc);
}

static void saturate(guchar *dest, gint width,
		     gint bpp, gint bpc, gdouble s_scale) {
  GimpRGB rgb;
  GimpHSV hsv;
  gint	  b = absolute(bpc);
  dest += b;	/* point to green before looping */
  while (width-- > 0) {
    rgb.r = get_pixel(dest-b, bpc);
    rgb.g = get_pixel(dest  , bpc);
    rgb.b = get_pixel(dest+b, bpc);
    gimp_rgb_to_hsv (&rgb, &hsv);
    hsv.s *= s_scale;
    if (hsv.s > 1.0)
      hsv.s = 1.0;
    gimp_hsv_to_rgb (&hsv, &rgb);
    set_pixel(dest-b, rgb.r, bpc);
    set_pixel(dest  , rgb.g, bpc);
    set_pixel(dest+b, rgb.b, bpc);
    dest += bpp;
  }
}

static void centerline(guchar *dest, gint width, gint bpp, gint bpc,
		       gint x, gint y, gint xc, gint yc) {
  gint    i, b = absolute(bpc);
  gdouble c = 1.0;
  dest += b;
  if (y == yc) {
    i = absolute(xc - x) %16;
    if (i < 8) c = 0.0;
    while (width-- > 0) {
      set_pixel(dest-b, c, bpc);
      set_pixel(dest  , c, bpc);
      set_pixel(dest+b, c, bpc);
      if (i-- < 0) {
	i = 7;
	if (c > 0)
	  c = 0.0;
	else
	  c = 1.0;
      }
      dest += bpp;
    }
    return;
  }
  if (y <= yc)
    y = yc - y;
  else
    y = y - yc;
  i = absolute(y) % 16;
  if (i < 8) c = 0.0;
  xc -= x;
  if (xc >= 0 && xc < width) {
    x = xc * bpp;
    set_pixel(dest+x-b, c, bpc);
    set_pixel(dest+x  , c, bpc);
    set_pixel(dest+x+b, c, bpc);
  }
  x = xc - y;
  if (x >= 0 && x < width) {
    x *= bpp;
    set_pixel(dest+x-b, c, bpc);
    set_pixel(dest+x  , c, bpc);
    set_pixel(dest+x+b, c, bpc);
  }
  x = xc + y;
  if (x >= 0 && x < width) {
    x *= bpp;
    set_pixel(dest+x-b, c, bpc);
    set_pixel(dest+x  , c, bpc);
    set_pixel(dest+x+b, c, bpc);
  }
}

static void fix_ca_region(guchar *srcPTR, guchar *dstPTR,
			  gint orig_width, gint orig_height,
			  gint bytes, gint bpc, FixCaParams *params,
			  gint x1, gint x2, gint y1, gint y2,
			  gboolean show_progress) {
  guchar *src[SOURCE_ROWS];
  gint    src_row[SOURCE_ROWS];
  gint    src_iter[SOURCE_ROWS];
  gint    b, i;
  guchar *dest;
  gint    x, y, x_center, y_center, max_dim;
  gdouble scale_blue, scale_red, scale_max;
  gdouble x_shift_max, x_shift_min;
  gint    band_1, band_2, band_adj;
#ifdef DEBUG_TIME
  double  sec;
  struct  timeval tv1, tv2;
  gettimeofday(&tv1, NULL);
#endif

  if (show_progress)
    gimp_progress_init(_("Shifting pixel components..."));

  /* Allocate buffers for reading, writing */
  for (i = 0; i < SOURCE_ROWS; ++i) {
    src[i] = g_new (guchar, orig_width * bytes);
    src_row[i] = ROW_INVALID;	/* Invalid row */
    src_iter[i] = ITER_INITIAL;	/* Oldest iteration */
  }
  dest = g_new (guchar, (x2-x1) * bytes);

  x_center = params->lens_x;
  y_center = params->lens_y;
  /* Scale to get source */
  if (x_center >= y_center)
    max_dim = x_center;
  else
    max_dim = y_center;
  if (orig_width - x_center > max_dim)
    max_dim = orig_width - x_center;
  if (orig_height - y_center > max_dim)
    max_dim = orig_height - y_center;
  scale_blue = max_dim / (max_dim + params->blue);
  scale_red = max_dim / (max_dim + params->red);

  /* Optimize by loading only parts of a row */
  if (scale_blue > scale_red)
    scale_max = scale_blue;
  else
    scale_max = scale_red;

  if (params->x_blue > params->x_red) {
    x_shift_min = params->x_red;
    x_shift_max = params->x_blue;
  } else {
    x_shift_min = params->x_blue;
    x_shift_max = params->x_red;
  }

  /* Horizontal band to load for each row */
  band_1 = scale(x1, x_center, orig_width, scale_max, x_shift_max);
  band_2 = scale(x2-1, x_center, orig_width, scale_max, x_shift_min);
  if (band_1 > x1)	/* Make sure green is also covered */
    band_1 = x1;
  if (band_2 < x2-1)
    band_2 = x2-1;

  /* Extra pixels needed for interpolation */
  if (params->interpolation != GIMP_INTERPOLATION_NONE) {
    if (band_1 > 0)
      --band_1;
    if (band_2 < orig_width-1)
      ++band_2;
  }
  /* More pixels needed for cubic interpolation */
  if (params->interpolation == GIMP_INTERPOLATION_CUBIC) {
    if (band_1 > 0)
      --band_1;
    if (band_2 < orig_width-1)
      ++band_2;
  }

  band_adj = band_1 * bytes;
  b = absolute (bpc);
#ifdef DEBUG_TIME
  printf("fix_ca_region(), xc=%d of %d yc=%d of %d b=%d, %d, %d\n",
	 x_center, orig_width, y_center, orig_height, bpc, b, bytes);
#endif

  for (y = y1; y < y2; ++y) {
    /* Get current row, for green channel */
    guchar *ptr;
    ptr = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
		    band_adj, band_1, band_2, y, y);

    /* Collect Green and Alpha channels all at once */
    memcpy(dest, &ptr[x1], (x2-x1)*bytes);

    if (params->interpolation == GIMP_INTERPOLATION_NONE) {
      guchar *ptr_blue, *ptr_red;
      gint    y_blue, y_red, x_blue, x_red;

      /* Get blue and red row */
      y_blue = scale(y, y_center, orig_height, scale_blue, params->y_blue);
      y_red = scale(y, y_center, orig_height, scale_red, params->y_red);
      ptr_blue = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			   band_adj, band_1, band_2, y_blue, y);
      ptr_red = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			  band_adj, band_1, band_2, y_red, y);

      for (x = x1; x < x2; ++x) {
	/* Blue and red channel */
	x_blue = scale(x, x_center, orig_width, scale_blue, params->x_blue);
	x_red = scale(x, x_center, orig_width, scale_red, params->x_red);

	memcpy(&dest[(x-x1)*bytes + 2*b], &ptr_blue[x_blue*bytes + 2*b], b);
	memcpy(&dest[(x-x1)*bytes], &ptr_red[x_red*bytes], b);
      }
    } else if (params->interpolation == GIMP_INTERPOLATION_LINEAR) {
      /* Pointer to pixel data rows y, y+1 */
      guchar *ptr_blue_1, *ptr_blue_2, *ptr_red_1, *ptr_red_2;
      /* Floating point row, fractional row */
      gdouble y_blue_d, y_red_d, d_y_blue, d_y_red;
      /* Integer row y */
      gint    y_blue_1, y_red_1;
      /* Floating point column, fractional column */
      gdouble x_blue_d, x_red_d, d_x_blue, d_x_red;
      /* Integer column x, x+1 */
      gint    x_blue_1, x_red_1, x_blue_2, x_red_2;

      /* Get blue and red row */
      y_blue_d = scale_d(y, y_center, orig_height, scale_blue, params->y_blue);
      y_red_d = scale_d(y, y_center, orig_height, scale_red, params->y_red);

      /* Integer and fractional row */
      y_blue_1 = floor(y_blue_d);
      y_red_1 = floor(y_red_d);
      d_y_blue = y_blue_d - y_blue_1;
      d_y_red = y_red_d - y_red_1;

      /* Load pixel data */
      ptr_blue_1 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			     band_adj, band_1, band_2, y_blue_1, y);
      ptr_red_1 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			    band_adj, band_1, band_2, y_red_1, y);
      if (y_blue_1 == orig_height-1)
	ptr_blue_2 = ptr_blue_1;
      else
	ptr_blue_2 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			       band_adj, band_1, band_2, y_blue_1+1, y);
      if (y_red_1 == orig_height-1)
	ptr_red_2 = ptr_red_1;
      else
	ptr_red_2 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			      band_adj, band_1, band_2, y_red_1+1, y);

      for (x = x1; x < x2; ++x) {
	/* Blue and red channel */
	x_blue_d = scale_d(x, x_center, orig_width, scale_blue, params->x_blue);
	x_red_d = scale_d(x, x_center, orig_width, scale_red, params->x_red);

	/* Integer and fractional column */
	x_blue_1 = floor(x_blue_d);
	x_red_1 = floor(x_red_d);
	d_x_blue = x_blue_d - x_blue_1;
	d_x_red = x_red_d - x_red_1;
	if (x_blue_1 == orig_width-1)
	  x_blue_2 = x_blue_1;
	else
	  x_blue_2 = x_blue_1 + 1;
	if (x_red_1 == orig_width-1)
	  x_red_2 = x_red_1;
	else
	  x_red_2 = x_red_1 + 1;

	/* Interpolation */
	bilinear((dest+((x-x1)*bytes+2*b)),
		 (ptr_blue_1+2*b), (ptr_blue_2+2*b), x_blue_1, x_blue_2,
		 bytes, bpc, d_x_blue, d_y_blue);
	bilinear((dest+((x-x1)*bytes)),
		 ptr_red_1, ptr_red_2, x_red_1, x_red_2,
		 bytes, bpc, d_x_red, d_y_red);
      }
    } else if (params->interpolation == GIMP_INTERPOLATION_CUBIC) {
      /* Pointer to pixel data rows y-1, y */
      guchar *ptr_blue_1, *ptr_blue_2, *ptr_red_1, *ptr_red_2;
      /* Pointer to pixel data rows y+1, y+2 */
      guchar *ptr_blue_3, *ptr_blue_4, *ptr_red_3, *ptr_red_4;
      /* Floating point row, fractional row */
      gdouble y_blue_d, y_red_d, d_y_blue, d_y_red;
      /* Integer row y */
      gint    y_blue_2, y_red_2;
      /* Floating point column, fractional column */
      gdouble x_blue_d, x_red_d, d_x_blue, d_x_red;
      /* Integer column x-1, x */
      gint    x_blue_1, x_red_1, x_blue_2, x_red_2;
      /* Integer column x+1, x+2 */
      gint    x_blue_3, x_red_3, x_blue_4, x_red_4;

      /* Get blue and red row */
      y_blue_d = scale_d(y, y_center, orig_height, scale_blue, params->y_blue);
      y_red_d = scale_d(y, y_center, orig_height, scale_red, params->y_red);

      y_blue_2 = floor(y_blue_d);
      y_red_2 = floor(y_red_d);
      d_y_blue = y_blue_d - y_blue_2;
      d_y_red = y_red_d - y_red_2;

      /* Row */
      ptr_blue_2 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			     band_adj, band_1, band_2, y_blue_2, y);
      ptr_red_2 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			    band_adj, band_1, band_2, y_red_2, y);

      /* Row - 1 */
      if (y_blue_2 == 0)
	ptr_blue_1 = ptr_blue_2;
      else
	ptr_blue_1 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			       band_adj, band_1, band_2, y_blue_2-1, y);
      if (y_red_2 == 0)
	ptr_red_1 = ptr_red_2;
      else
	ptr_red_1 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			      band_adj, band_1, band_2, y_red_2-1, y);

      /* Row + 1 */
      if (y_blue_2 == orig_height-1)
	ptr_blue_3 = ptr_blue_2;
      else
	ptr_blue_3 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			       band_adj, band_1, band_2, y_blue_2+1, y);
      if (y_red_2 == orig_height-1)
	ptr_red_3 = ptr_red_2;
      else
	ptr_red_3 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			      band_adj, band_1, band_2, y_red_2+1, y);

      /* Row + 2 */
      if (y_blue_2 == orig_height-1)
	ptr_blue_4 = ptr_blue_2;
      else if (y_blue_2 == orig_height-2)
	ptr_blue_4 = ptr_blue_3;
      else
	ptr_blue_4 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			       band_adj, band_1, band_2, y_blue_2+2, y);
      if (y_red_2 == orig_height-1)
	ptr_red_4 = ptr_red_2;
      else if (y_red_2 == orig_height-2)
	ptr_red_4 = ptr_red_3;
      else
	ptr_red_4 = load_data(orig_width, bytes, srcPTR, src, src_row, src_iter,
			      band_adj, band_1, band_2, y_red_2+2, y);

      for (x = x1; x < x2; ++x) {
	double y1, y2, y3, y4;

	/* Blue and red channel */
	x_blue_d = scale_d(x, x_center, orig_width, scale_blue, params->x_blue);
	x_red_d = scale_d(x, x_center, orig_width, scale_red, params->x_red);

	x_blue_2 = floor (x_blue_d);
	x_red_2 = floor (x_red_d);
	d_x_blue = x_blue_d - x_blue_2;
	d_x_red = x_red_d - x_red_2;

	/* Column - 1 */
	if (x_blue_2 == 0)
	  x_blue_1 = x_blue_2;
	else
	  x_blue_1 = x_blue_2 - 1;
	if (x_red_2 == 0)
	  x_red_1 = x_red_2;
	else
	  x_red_1 = x_red_2 - 1;

	/* Column + 1 */
	if (x_blue_2 == orig_width-1)
	  x_blue_3 = x_blue_2;
	else
	  x_blue_3 = x_blue_2 + 1;
	if (x_red_2 == orig_width-1)
	  x_red_3 = x_red_2;
	else
	  x_red_3 = x_red_2 + 1;

	/* Column + 2 */
	if (x_blue_3 == orig_width-1)
	  x_blue_4 = x_blue_3;
	else
	  x_blue_4 = x_blue_3 + 1;
	if (x_red_3 == orig_width-1)
	  x_red_4 = x_red_3;
	else
	  x_red_4 = x_red_3 + 1;

	y1 = cubicY(ptr_blue_1+2*b, bytes, bpc, d_x_blue,
		    x_blue_1, x_blue_2, x_blue_3, x_blue_4);
	y2 = cubicY(ptr_blue_2+2*b, bytes, bpc, d_x_blue,
		    x_blue_1, x_blue_2, x_blue_3, x_blue_4);
	y3 = cubicY(ptr_blue_3+2*b, bytes, bpc, d_x_blue,
		    x_blue_1, x_blue_2, x_blue_3, x_blue_4);
	y4 = cubicY(ptr_blue_4+2*b, bytes, bpc, d_x_blue,
		    x_blue_1, x_blue_2, x_blue_3, x_blue_4);
	cubicX((dest+(x-x1)*bytes+2*b), bytes, bpc, d_y_blue, y1, y2, y3, y4);

	y1 = cubicY(ptr_red_1, bytes, bpc, d_x_red,
		    x_red_1, x_red_2, x_red_3, x_red_4);
	y2 = cubicY(ptr_red_2, bytes, bpc, d_x_red,
		    x_red_1, x_red_2, x_red_3, x_red_4);
	y3 = cubicY(ptr_red_3, bytes, bpc, d_x_red,
		    x_red_1, x_red_2, x_red_3, x_red_4);
	y4 = cubicY(ptr_red_4, bytes, bpc, d_x_red,
		    x_red_1, x_red_2, x_red_3, x_red_4);
	cubicX((dest+(x-x1)*bytes), bytes, bpc, d_y_red, y1, y2, y3, y4);
      }
    }

    if (!show_progress) {
      if (params->saturation != 0.0)
	saturate(dest, x2-x1, bytes, bpc, 1+params->saturation/100);

      centerline(dest, x2-x1, bytes, bpc, x1, y, x_center, y_center);
    }

    set_data(dstPTR, dest, bytes, orig_width, x1, y, (x2-x1));

    if (show_progress && ((y-y1) % 8 == 0))
      gimp_progress_update((gdouble) (y-y1)/(y2-y1));
  }

  if (show_progress)
    gimp_progress_update(0.0);

  for (i = 0; i < SOURCE_ROWS; ++i)
    g_free(src[i]);
  g_free(dest);

#ifdef DEBUG_TIME
  gettimeofday(&tv2, NULL);

  sec = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
  printf ("fix-ca Elapsed time: %.2f\n", sec);
#endif
}

static void fix_ca_help(const gchar *help_id, gpointer help_data) {
  gimp_message(_("The image to modify is in RGB format.  Color precision "
		"can be float, double, 8, 16, 32, 64.  The green pixels "
		"are kept stationary, and you can shift the red and blue "
		"colors within a range of {-10..+10} pixels.\n\n"
		"Lateral Chromatic Aberration is due to camera lens(es) "
		"with no aberration at the lens center, and increasing "
		"gradually toward the edges of the image.\n\n"
		"Directional X and Y axis aberrations are a flat amount "
		"of aberration due to image seen through something like "
		"glass, water, or another medium of different density.  "
		"You can shift pixels up/left {-10..+10} down/right.\n\n"
		"Lateral aberration correction is applied first, since "
		"the lens(es) are closest to the film or image sensor, "
		"and directional corrections applied last since this is "
		"the furthest away from the camera."));
}
