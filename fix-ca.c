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
#define _(x) x
#define N_(x) x
#endif

#ifdef DEBUG_TIME
# include <sys/time.h>
# include <stdio.h>
#endif

#define PLUG_IN_PROC	"plug-in-fix-ca"
#define PLUG_IN_ROLE	"gimp-fix-ca"
#define PLUG_IN_BINARY	"fix-ga"
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

typedef struct _FixCa FixCa;
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
  gboolean reset_values;
  /* private, internal use */
  GimpDrawable *drawable;
  gint    Xsize;
  gint    Ysize;
  gint    bpp;
  gint    bpc;
  guchar *srcImg;
  guchar *destImg;
  guchar *prevImg;
  GimpPreview *preview;
  GtkWidget *previewarea;
  gint    XPsize;
  gint    YPsize;
} FixCaParams;

/* Declare local functions. */
#define FIXCA_TYPE  (fixca_get_type())
#define FIXCA (obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FIXCA_TYPE, FixCa))

GType                   fixca_get_type         (void) G_GNUC_CONST;

static GList          * fixca_query_procedures (GimpPlugIn          *plug_in);
static GimpProcedure  * fixca_create_procedure (GimpPlugIn          *plug_in,
                                                const gchar         *name);

static GimpValueArray * fixca_run              (GimpProcedure       *procedure,
                                                GimpRunMode          run_mode,
                                                GimpImage           *image,
                                                gint                 n_drawables,
                                                GimpDrawable       **drawables,
                                                GimpProcedureConfig *proc_config,
                                                gpointer             run_data);

static gboolean         fixca_dialog           (GimpProcedure       *procedure,
                                                GimpProcedureConfig *proc_config,
                                                FixCaParams         *params);

static void             fixca_region           (FixCaParams         *params,
                                                gint                 x1,
                                                gint                 x2,
                                                gint                 y1,
                                                gint                 y2,
                                                gboolean             show_progress);

static void             fixca_help             (const gchar         *help_id,
                                                gpointer             help_data);

static void             preview_update         (GimpPreview         *preview,
//static void             preview_update         (GtkWidget           *preview,
                                                FixCaParams         *params);

static void             set_default_settings   (FixCaParams         *params);

static gboolean         get_from_config        (GimpProcedureConfig *proc_config,
                                                FixCaParams         *params);


G_DEFINE_TYPE (FixCa, fixca, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FIXCA_TYPE)
//DEFINE_STD_SET_I18N; ***don't use - 3rd-party locale external to gimp3

static void
fixca_class_init (FixCaClass *klass)
{
  GimpPlugInClass *plug_in_class  = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = fixca_query_procedures;
  plug_in_class->create_procedure = fixca_create_procedure;
  //plug_in_class->set_i18n       = STD_SET_I18N;
}

static void
fixca_init (FixCa *fixca)
{
}

static GList *
fixca_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
fixca_create_procedure (GimpPlugIn *plug_in, const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (!strcmp (name, PLUG_IN_PROC)) {

    gegl_init (NULL, NULL);

    procedure = gimp_image_procedure_new (plug_in, name,
                                          GIMP_PDB_PROC_TYPE_PLUGIN,
                                          fixca_run, NULL, NULL);

    gimp_procedure_set_image_types (procedure, "RGB*");
    gimp_procedure_set_sensitivity_mask (procedure,
                                         GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

#ifdef xxHAVE_GETTEXT
    /* Initialize i18n support */
    //setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
    //textdomain (GETTEXT_PACKAGE);
#endif

    gimp_procedure_set_menu_label (procedure, _("Chromatic Aberration..."));
    gimp_procedure_add_menu_path (procedure, _("<Image>/Filters/Colors"));
    gimp_procedure_set_documentation (procedure,
                                      _("Fix chromatic aberration"),
                                      _("Fix chromatic aberration caused by "
                                      "imperfect lens.  It works by shifting "
                                      "red and blue components of image pixels "
                                      "in the specified amounts."),
                                      PLUG_IN_PROC);
    gimp_procedure_set_attribution (procedure,
                                    "Kriang Lerdsuwanakij",
                                    "Kriang Lerdsuwanakij 2006,2007, Jose Da Silva 2022..2024",
                                    "2006 - 2024");

    GIMP_PROC_ARG_DOUBLE (procedure, "blue",
                          _("_Blue"), _("Blue amount (lateral)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE (procedure, "red",
                          _("_Red"), _("Red amount (lateral)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT (procedure, "lens_x",
                       _("lens_X"), _("Lens center X (lateral)"),
                       0, GIMP_MAX_IMAGE_SIZE-1, 0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT (procedure, "lens_y",
                       _("lens_Y"), _("Lens center Y (lateral)"),
                       0, GIMP_MAX_IMAGE_SIZE-1, 0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_INT (procedure, "interpolation",
                       _("_Interpolation"),
                       _("Interpolation 0=None/1=Linear/2=Cubic"),
                       0, 2, 1, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE (procedure, "x_blue",
                          _("X B_lue"), _("Blue amount, X axis (directional)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE (procedure, "x_red",
                          _("X R_ed"), _("Red amount, X axis (directional)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE (procedure, "y_blue",
                          _("Y Bl_ue"), _("Blue amount, Y axis (directional)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_DOUBLE (procedure, "y_red",
                          _("Y Re_d"), _("Red amount, Y axis (directional)"),
                          -INPUT_MAX, INPUT_MAX, 0.0, G_PARAM_READWRITE);
    GIMP_PROC_ARG_BOOLEAN (procedure, "reset_values",
                           _("_Reset all values to default"),
                           _("Reset all values to default"),
                           FALSE,
                           G_PARAM_READWRITE);
    /* GUI-only arguments */
//    GIMP_PROC_AUX_ARG_DOUBLE (procedure, "saturation",
//                              _("Preview _saturation"),
//                              _("Saturate colors to help you see overlaps"),
//                              -100.0, 100.0, 0.0, G_PARAM_READWRITE);
  }

  return procedure;
}

static GimpValueArray *fixca_run(GimpProcedure	      *procedure,
				 GimpRunMode	       run_mode,
				 GimpImage	      *image,
				 gint		       n_drawables,
				 GimpDrawable	     **drawables,
				 GimpProcedureConfig  *proc_config,
				 gpointer	       run_data) {
  const Babl        *format;
  GeglBuffer        *srcBuf;
  GeglBuffer        *shadow;
  GimpDrawable      *drawable;
  FixCaParams        fix_ca_params;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error = NULL;
  gint               x, y, width, height, sizeImg;

#ifdef xxHAVE_GETTEXT
  /* Initialize i18n support */
  //setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, gimp_locale_directory());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
  //textdomain(GETTEXT_PACKAGE);
#endif

  if (n_drawables != 1) {
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 _("Procedure '%s' only works with one drawable."),
                 gimp_procedure_get_name (procedure));

    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }
  fix_ca_params.drawable = drawable = drawables[0];

  if (!gimp_drawable_is_rgb (drawable)) {
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 _("Procedure '%s' only works with RGB or RGBA."),
                 gimp_procedure_get_name (procedure));

    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }

  fix_ca_params.Xsize  = gimp_drawable_get_width (drawable);
  fix_ca_params.Ysize  = gimp_drawable_get_height (drawable);
  fix_ca_params.bpp    = gimp_drawable_get_bpp (drawable);
  fix_ca_params.srcImg = fix_ca_params.destImg = fix_ca_params.prevImg = NULL;
  set_default_settings (&fix_ca_params);

  /* figure-out bytes per color precision */
  switch (gimp_image_get_precision (image)) {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_NON_LINEAR:
    case GIMP_PRECISION_U8_PERCEPTUAL:
      fix_ca_params.bpc = 1;
      break;
    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_NON_LINEAR:
    case GIMP_PRECISION_U16_PERCEPTUAL:
      fix_ca_params.bpc = 2;
      break;
    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_NON_LINEAR:
    case GIMP_PRECISION_U32_PERCEPTUAL:
      fix_ca_params.bpc = 4;
      break;
    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_NON_LINEAR:
    case GIMP_PRECISION_DOUBLE_PERCEPTUAL:
      fix_ca_params.bpc = -8; /* IEEE 754 double precision */
      break;
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_NON_LINEAR:
    case GIMP_PRECISION_FLOAT_PERCEPTUAL:
      fix_ca_params.bpc = -4; /* IEEE 754 single precision */
      break;
    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_NON_LINEAR:
    case GIMP_PRECISION_HALF_PERCEPTUAL:
      fix_ca_params.bpc = -2; /* IEEE 754 half precision */
      break;
    default:
      if (fix_ca_params.bpp == 24 || fix_ca_params.bpp == 32) {
        fix_ca_params.bpc = 8; /* 64bit RBC/RBCA */
      } else {
        g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                     _("Procedure '%s' cannot use this color precision."),
                     gimp_procedure_get_name (procedure));
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 error);
      }
  }
#ifdef DEBUG_TIME
  printf("Xsize=%d Ysize=%d, ",
	 fix_ca_params.Xsize, fix_ca_params.Ysize);
  printf("bytes per pixel=%d, bytes per color=%d\n",
	 fix_ca_params.bpp, fix_ca_params.bpc);
#endif

  /* prepare to draw or preview, fetch values and image */
  sizeImg = fix_ca_params.Xsize * fix_ca_params.Ysize * fix_ca_params.bpp;
  fix_ca_params.srcImg  = g_new (guchar, sizeImg);
  fix_ca_params.destImg = g_new (guchar, sizeImg);
  if (fix_ca_params.srcImg == NULL || fix_ca_params.destImg == NULL) {
    /* TODO: REDO gimp-fix-ca to work with super-large-pics */
    if (fix_ca_params.destImg != NULL) g_free(fix_ca_params.destImg);
    if (fix_ca_params.srcImg  != NULL) g_free(fix_ca_params.srcImg);
    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                 _("Procedure '%s', not enough RAM."),
                 gimp_procedure_get_name (procedure));
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CALLING_ERROR,
                                             error);
  }

//  if (!get_from_config (&proc_config, &fix_ca_params)) {
//    g_set_error (&error, GIMP_PLUG_IN_ERROR, 0, _("Parameter out of range!"));
//    status = GIMP_PDB_CALLING_ERROR;
//  }

  format = gimp_drawable_get_format (drawable);
  //gegl_init (NULL, NULL);
  srcBuf = gimp_drawable_get_buffer (drawable);
  gegl_buffer_get (srcBuf,
                   GEGL_RECTANGLE (0, 0, fix_ca_params.Xsize, fix_ca_params.Ysize),
                   1.0, format, fix_ca_params.srcImg, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  /* In interactive mode, display a dialog to advertise the exercise. */
  if (run_mode == GIMP_RUN_INTERACTIVE) {
    if (!fixca_dialog (procedure, &proc_config, &fix_ca_params)) {
      status = GIMP_PDB_CANCEL;
    }
  }
#ifdef DEBUG_TIME
  printf("Xsize=%d Ysize=%d, ",
	 fix_ca_params.Xsize, fix_ca_params.Ysize);
  printf("bytes per pixel=%d, bytes per color=%d\n",
	 fix_ca_params.bpp, fix_ca_params.bpc);
#endif
#ifdef DEBUG_TIME
  printf("done fixca_dialog\n");
#endif

//  if (status == GIMP_PDB_SUCCESS && !get_from_config (&proc_config, &fix_ca_params)) {
//    g_set_error(&error, GIMP_PLUG_IN_ERROR, 0, _("Parameter out of range!"));
//    status = GIMP_PDB_CALLING_ERROR;
//  }

  /* Modify image according to input params */
  if (status == GIMP_PDB_SUCCESS &&
      gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height)) {

#ifdef DEBUG_TIME
      double	sec;
      struct timeval tv1, tv2;
      gettimeofday (&tv1, NULL);

      printf ("Start fixca(), x=%d y=%d width=%d height=%d\n",
	     x, y, width, height);
#endif

      shadow = gimp_drawable_get_shadow_buffer(drawable);

      /* adjust pixel regions from srcImg to destImg, according to params */
      fixca_region (&fix_ca_params, x, (x+width), y, (y+height), TRUE);
#ifdef DEBUG_TIME
  printf ("finished doing fixca_region\n");
  printf ("blue=%g red=%g lens_x=%g lens_y=%g Xsize=%d/%d Ysize=%d/%d",
          fix_ca_params.blue, fix_ca_params.red, fix_ca_params.lens_x, fix_ca_params.lens_y, fix_ca_params.Xsize, width, fix_ca_params.Ysize, height);
  printf ("x_blue=%g x_red=%g x_blue=%g x_red=%g\n",
          fix_ca_params.x_blue, fix_ca_params.x_red, fix_ca_params.y_blue, fix_ca_params.y_red);
  printf ("interpolation=%d saturation=%g reset values=%d\n",
          fix_ca_params.interpolation, fix_ca_params.saturation, fix_ca_params.reset_values);
#endif

      gegl_buffer_set (shadow, GEGL_RECTANGLE(x, y, width, height),
                       0, format, fix_ca_params.destImg, GEGL_AUTO_ROWSTRIDE);

      gegl_buffer_flush (shadow);
      gimp_drawable_merge_shadow (drawable, TRUE);
      gimp_drawable_update (drawable, x, y, width, height);
      g_object_unref (shadow);

#ifdef DEBUG_TIME
      gettimeofday (&tv2, NULL);

      sec = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
      printf ("End fixca(), Elapsed time: %.2f\n", sec);
#endif
  }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  g_object_unref(srcBuf);
  //gegl_exit ();
  g_free (fix_ca_params.destImg);
  g_free (fix_ca_params.srcImg);
#ifdef DEBUG_TIME
  printf ("Program end.\n");
#endif
  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static void
set_default_settings (FixCaParams *params)
{
  params->blue   = 0.0;
  params->red    = 0.0;
  params->lens_x = round (params->Xsize/2);
  params->lens_y = round (params->Ysize/2);
  params->update_preview = TRUE;
  params->interpolation = GIMP_INTERPOLATION_NONE;
  params->saturation = 0.0;
  params->x_blue = 0.0;
  params->x_red  = 0.0;
  params->y_blue = 0.0;
  params->y_red  = 0.0;
  params->reset_values = FALSE;
}

static gboolean
get_from_config (GimpProcedureConfig *proc_config, FixCaParams *params)
{
#ifdef DEBUG_TIME
  printf("start get_from_config\n");
#endif

  g_object_get (proc_config,
                "blue",          &params->blue,
                "red",           &params->red,
                "lens_x",        &params->lens_x,
                "lens_y",        &params->lens_y,
                "interpolation", &params->interpolation,
                "x_blue",        &params->x_blue,
                "x_red",         &params->x_red,
                "y_blue",        &params->y_blue,
                "y_red",         &params->y_red,
                "reset_values",  &params->reset_values,
                NULL);

#ifdef DEBUG_TIME
  printf("loaded config\n");
  printf("blue=%g red=%g lens_x=%g lens_y=%g ",
	 params->blue, params->red, params->lens_x, params->lens_y);
  printf("x_blue=%g x_red=%g x_blue=%g x_red=%g\n",
	 params->x_blue, params->x_red, params->y_blue, params->y_red);
  printf("interpolation=%d saturation=%g reset values=%d\n",
	 params->interpolation, params->saturation, params->reset_values);
#endif

  if (params->reset_values)
    set_default_settings (&params);

  if (params->blue   < -INPUT_MAX || \
      params->blue   >  INPUT_MAX || \
      params->red    < -INPUT_MAX || \
      params->red    >  INPUT_MAX || \
      params->lens_x <  0 || \
      params->lens_x >= params->Xsize || \
      params->lens_y <  0 || \
      params->lens_y >= params->Ysize || \
      params->interpolation < 0 || \
      params->interpolation > 2 || \
      params->x_blue < -INPUT_MAX || \
      params->x_blue >  INPUT_MAX || \
      params->x_red  < -INPUT_MAX || \
      params->x_red  >  INPUT_MAX || \
      params->y_blue < -INPUT_MAX || \
      params->y_blue >  INPUT_MAX || \
      params->y_red  < -INPUT_MAX || \
      params->y_red  >  INPUT_MAX)
    return FALSE;

  return TRUE;
}

static gdouble
get_pixel (guchar *ptr, gint bpc)
{
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

static void
set_pixel (guchar *dest, gdouble d, gint bpc)
{
  /* input value is in the range of [0.0..1.0]. */
  if (bpc == 1) {
    *dest = round (d * 255);
  }  else if (bpc == 2) {
    uint16_t *p = (uint16_t *)(dest);
    *p = round (d * 65535);
  } else if (bpc == 4) {
    uint32_t *p = (uint32_t *)(dest);
    *p = round (d * 4294967295);
  } else if (bpc == 8) {
    uint64_t *p = (uint64_t *)(dest);
    *p = roundl (d * 18446744073709551615L);
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

static int
round_nearest (gdouble d)
{
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

static int
absolute (gint i)
{
  if (i >= 0)
    return i;
  else
    return -i;
}

void
preview_update (GimpPreview *preview, FixCaParams *params)
//preview_update (GtkWidget *preview, FixCaParams *params)
{
  gint    b, i, j, x, y, width, height;
  gdouble d;

  gimp_preview_get_position (preview, &x, &y);
  gimp_preview_get_size (preview, &width, &height);
#ifdef DEBUG_TIME
  printf ("preview_update(), x=%d y=%d w=%d h=%d bpp=%d bcp=%d\n",
          x, y, width, height, params->bpp, params->bpc);
#endif

  fixca_region (params, 0, params->Xsize, y, (y + height), FALSE);
#ifdef DEBUG_TIME
  printf ("..continue update_preview\n");
#endif

  b = absolute (params->bpc);
  for (i = 0; i < height; i++) {
    if (b == 1) {
      memcpy (&params->prevImg[width * i * params->bpp],
              &params->destImg[(params->Xsize * (y + i) + x) * params->bpp],
              width * params->bpp);
    } else {
      for (j = 0; j < width*params->bpp/b; j++) {
        d = get_pixel (&params->destImg[(params->Xsize*(y+i)+x)*params->bpp+j*b], params->bpc);
        set_pixel (&params->prevImg[width*i*params->bpp/b+j], d, 1);
      }
    }
  }

  gimp_preview_draw_buffer (preview, params->prevImg, width * params->bpp/b);
  //gtk_widget_queue_draw (params->previewarea);
#ifdef DEBUG_TIME
  printf ("done update_preview\n");
#endif
}

static void
gfix_scale_entry_update (GimpLabelSpin *entry,
                         gdouble       *value)
{
  *value = gimp_label_spin_get_value (entry);
}

static void
gfix_combo_entry_update (GtkWidget *widget,
                         gint      *value)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);
}

static gboolean
fixca_dialog (GimpProcedure       *procedure,
              GimpProcedureConfig *config,
              FixCaParams         *params)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *combo;
  GtkWidget *preview;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *adj;
  GtkWidget *button;
  gboolean   run;
  gchar     *title;
  gint       x, y, width, height;

  gimp_ui_init (PLUG_IN_BINARY);

  title = g_strdup_printf (_("Chromatic Aberration"));
  dialog = gimp_dialog_new (title, "fix_ca",
                            NULL, (GtkDialogFlags) 0,
                            fixca_help, PLUG_IN_PROC,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,
                            NULL);
  g_free (title);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

/***
  preview = gimp_drawable_preview_new_from_drawable(params->drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  gimp_preview_get_position(preview, &x, &y);
  gimp_preview_get_size(preview, &width, &height);
  params->prevImg = g_new(guchar, width * height * params->bpp);
/**
  preview = gimp_drawable_preview_new_from_drawable(params->drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  params->previewarea = preview;

  gimp_preview_get_position(preview, &x, &y);
  gimp_preview_get_size(preview, &width, &height);
  params->prevImg = g_new(guchar, width * height * params->bpp);

  g_signal_connect(preview, "invalidated",
		   G_CALLBACK(preview_update),
		   params);
/***/
//  grid = gtk_grid_new ();
//  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
//  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  //gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
//  gtk_widget_show (grid);

  adj = gimp_scale_entry_new (_("_Saturation:"),
                             params->saturation, -100.0, 100.0, 0);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN(adj), 1, 10);
  gimp_help_set_help_data (adj,
                           _("Saturate colors in preview window to help you see overlaps"),
                           NULL);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->saturation));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  combo = gimp_int_combo_box_new (_("None (Fastest)"), GIMP_INTERPOLATION_NONE,
                                  _("Linear"),         GIMP_INTERPOLATION_LINEAR,
                                  _("Cubic (Best)"),   GIMP_INTERPOLATION_CUBIC,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), params->interpolation);
  gimp_help_set_help_data (combo,
                           _("Method of how to move Blue and Red Pixels"),
                           NULL);
  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &(params->interpolation));
  gtk_box_pack_start (GTK_BOX (main_vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  frame = gimp_frame_new (_("Lateral"));
  gimp_help_set_help_data (frame,
                           _("Do corrections for lens affected chromatic aberration"),
                           NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  adj = gimp_scale_entry_new (_("_Blue:"), params->blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->blue));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  adj = gimp_scale_entry_new (_("_Red:"), params->red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->red));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  adj = gimp_scale_entry_new (_("Lens_X:"), params->lens_x, 0, params->Xsize-1, 0);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 10, 100);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->lens_x));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  adj = gimp_scale_entry_new (_("Lens_Y:"), params->lens_y, 0, params->Ysize-1, 0);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 10, 100);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->lens_y));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  //button = gtk_check_button_new_with_mnemonic(_("Reset Lens Center"));
  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
  //                              data->use_full_page);
  //gtk_box_pack_start(GTK_BOX(main_vbox), button, FALSE, FALSE, 0);
  //g_signal_connect(button, "toggled",
  //                  G_CALLBACK(print_size_info_use_full_page_toggled),
  //                  NULL);
  //gtk_widget_show(button);

  //button = gtk_check_button_new_with_mnemonic(_("_Draw Crop Marks"));
  //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button),
  //                              data->draw_crop_marks);
  //gtk_box_pack_start(GTK_BOX(main_vbox), button, FALSE, FALSE, 0);
  //g_signal_connect(button, "toggled",
  //                  G_CALLBACK(print_draw_crop_marks_toggled),
  //                  NULL);
  //gtk_widget_show(button);

  frame = gimp_frame_new (_("Directional, X axis"));
  gimp_help_set_help_data (frame,
                           _("Do flat directional corrections along the X axis"),
                           NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  adj = gimp_scale_entry_new (_("B_lue:"), params->x_blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->x_blue));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  adj = gimp_scale_entry_new (_("R_ed:"), params->x_red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->x_red));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  frame = gimp_frame_new (_("Directional, Y axis"));
  gimp_help_set_help_data (frame,
                           _("Do flat directional corrections along the Y axis"),
                           NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  adj = gimp_scale_entry_new (_("Bl_ue:"), params->y_blue, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gfix_scale_entry_update),
                    &(params->y_blue));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK (gimp_preview_invalidate),
//			   preview);
/***/
  adj = gimp_scale_entry_new (_("Re_d:"), params->y_red, -INPUT_MAX, INPUT_MAX, 1);
  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (adj), 0.1, 0.5);
  gtk_box_pack_start (GTK_BOX (main_vbox), adj, FALSE, FALSE, 0);
  gtk_widget_show (adj);

  g_signal_connect (GIMP_LABEL_SPIN (adj), "value_changed",
                   G_CALLBACK (gfix_scale_entry_update),
                   &(params->y_red));
//  g_signal_connect_swapped(adj, "value_changed",
//			   G_CALLBACK(gimp_preview_invalidate),
//			   preview);

  gtk_widget_show (dialog);
  run = (gimp_dialog_run (GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);

//  g_free (params->prevImg);

/***
  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Chromatic Aberration"));
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
					    GTK_RESPONSE_OK,
					    GTK_RESPONSE_CANCEL,
					    -1);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "blue", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "red", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "lens_x", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "lens_y", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "interpolation", 1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "x_blue", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "x_red", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "y_blue", 0.1);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "y_red", 0.1);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);
  gtk_widget_show(dialog);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));
  gtk_widget_destroy (dialog);
***/

  return run;
}

static int
scale (gint i, gint center, gint size, gdouble scale_val, gdouble shift_val)
{
  gdouble d = (i - center) * scale_val + center - shift_val;
  gint    j = round_nearest (d);
  if (j <= 0)
    return 0;
  else if (j < size)
    return j;
  else
    return size-1;
}

static double
scale_d (gint i, gint center, gint size, gdouble scale_val, gdouble shift_val)
{
  gdouble d = (i - center) * scale_val + center - shift_val;
  if (d <= 0.0)
    return 0.0;
  else if (d >= size-1)
    return size-1;
  else
    return d;
}

static guchar *
load_data (gint fullWidth, gint bpp, guchar *srcPTR,
           guchar *src[SOURCE_ROWS], gint src_row[SOURCE_ROWS],
           gint src_iter[SOURCE_ROWS], gint band_adj,
           gint band_left, gint band_right, gint y, gint iter)
{
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
  memcpy (&src[row_best][i], &srcPTR[x + i], l - i);
  src_row[row_best] = y;
  src_iter[row_best] = iter;
  return src[row_best];
}

static void
set_data (guchar *dest, FixCaParams *params, gint xstart, gint yrow, gint width)
{
  gint l, x;
  x = (params->Xsize * yrow + xstart) * params->bpp;
  l = width * params->bpp;
  memcpy (&params->destImg[x], dest, l);
}

static gdouble
clip_d (gdouble d)
{
  if (d <= 0.0)
    return 0.0;
  if (d >= 1.0)
    return 1.0;
  return d;
}

static void bilinear(guchar *dest,
                     guchar *yrow0, guchar *yrow1, gint x0, gint x1,
                     gint bpp, gint bpc, gdouble dx, gdouble dy)
{
  gdouble d, x0y0, x1y0, x0y1, x1y1;
  x0y0 = get_pixel (&yrow0[x0*bpp], bpc);
  x1y0 = get_pixel (&yrow0[x1*bpp], bpc);
  x0y1 = get_pixel (&yrow1[x0*bpp], bpc);
  x1y1 = get_pixel (&yrow1[x1*bpp], bpc);
  d = (1-dy) * (x0y0 + dx * (x1y0-x0y0))
       + dy  * (x0y1 + dx * (x1y1-x0y1));
  set_pixel (dest, clip_d(d), bpc);
}

static gdouble cubicY(guchar *yrow, gint bpp, gint bpc, gdouble dx,
                      gint m1, gint x0, gint p1, gint p2)
{
  /* Catmull-Rom from Gimp gimpdrawable-transform.c */
  gdouble d, xm1, x, xp1, xp2;
  xm1 = get_pixel (&yrow[m1*bpp], bpc);
  x   = get_pixel (&yrow[x0*bpp], bpc);
  xp1 = get_pixel (&yrow[p1*bpp], bpc);
  xp2 = get_pixel (&yrow[p2*bpp], bpc);
  d = ((( ( - xm1 + 3 * x - 3 * xp1 + xp2 ) * dx +
        ( 2 * xm1 - 5 * x + 4 * xp1 - xp2 ) ) * dx +
          ( - xm1 + xp1 ) ) * dx + (x + x) ) / 2.0;
  return d;
}

static void cubicX(guchar *dest, gint bpp, gint bpc, gdouble dy,
                   gdouble ym1, gdouble y, gdouble yp1, gdouble yp2)
{
  /* Catmull-Rom from Gimp gimpdrawable-transform.c */
  gdouble d;
  d = ((( ( - ym1 + 3 * y - 3 * yp1 + yp2 ) * dy +
        ( 2 * ym1 - 5 * y + 4 * yp1 - yp2 ) ) * dy +
          ( - ym1 + yp1 ) ) * dy + (y + y) ) / 2.0;
  set_pixel (dest, clip_d(d), bpc);
}

static void
saturate(guchar *dest, gint width,
         gint bpp, gint bpc, gdouble s_scale)
{
  GimpRGB rgb;
  GimpHSV hsv;
  gint	  b = absolute (bpc);
  dest += b;	/* point to green before looping */
  while (width-- > 0) {
    rgb.r = get_pixel (dest-b, bpc);
    rgb.g = get_pixel (dest  , bpc);
    rgb.b = get_pixel (dest+b, bpc);
    gimp_rgb_to_hsv (&rgb, &hsv);
    hsv.s *= s_scale;
    if (hsv.s > 1.0)
      hsv.s = 1.0;
    gimp_hsv_to_rgb (&hsv, &rgb);
    set_pixel (dest-b, rgb.r, bpc);
    set_pixel (dest  , rgb.g, bpc);
    set_pixel (dest+b, rgb.b, bpc);
    dest += bpp;
  }
}

static void
centerline(guchar *dest, gint width, gint bpp, gint bpc,
           gint x, gint y, gint xc, gint yc)
{
  gint    i, b = absolute (bpc);
  gdouble c = 1.0;
  dest += b;
  if (y == yc) {
    i = absolute (xc - x) %16;
    if (i < 8) c = 0.0;
    while (width-- > 0) {
      set_pixel (dest-b, c, bpc);
      set_pixel (dest  , c, bpc);
      set_pixel (dest+b, c, bpc);
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
  i = absolute (y) % 16;
  if (i < 8) c = 0.0;
  xc -= x;
  if (xc >= 0 && xc < width) {
    x = xc * bpp;
    set_pixel (dest+x-b, c, bpc);
    set_pixel (dest+x  , c, bpc);
    set_pixel (dest+x+b, c, bpc);
  }
  x = xc - y;
  if (x >= 0 && x < width) {
    x *= bpp;
    set_pixel (dest+x-b, c, bpc);
    set_pixel (dest+x  , c, bpc);
    set_pixel (dest+x+b, c, bpc);
  }
  x = xc + y;
  if (x >= 0 && x < width) {
    x *= bpp;
    set_pixel (dest+x-b, c, bpc);
    set_pixel (dest+x  , c, bpc);
    set_pixel (dest+x+b, c, bpc);
  }
}

static void
fixca_region (FixCaParams *params,
              gint x1, gint x2, gint y1, gint y2,
              gboolean show_progress)
{
  guchar *srcPTR = params->srcImg;
  guchar *src[SOURCE_ROWS];
  gint    src_row[SOURCE_ROWS];
  gint    src_iter[SOURCE_ROWS];
  gint orig_width  = params->Xsize;
  gint orig_height = params->Ysize;
  gint bytes = params->bpp;
  gint bpc = params->bpc;
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
  printf ("fixca_region(), xc=%d of %d yc=%d of %d bpc=%d, b+=%d, bytes=%d\n",
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
      y_blue_1 = floor (y_blue_d);
      y_red_1 = floor (y_red_d);
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
	x_blue_1 = floor (x_blue_d);
	x_red_1 = floor (x_red_d);
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

      y_blue_2 = floor (y_blue_d);
      y_red_2 = floor (y_red_d);
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
        saturate (dest, x2-x1, bytes, bpc, 1+params->saturation/100);

      centerline (dest, x2-x1, bytes, bpc, x1, y, x_center, y_center);
    }

    set_data (dest, params, x1, y, (x2-x1));

    if (show_progress && ((y-y1) % 8 == 0))
      gimp_progress_update((gdouble) (y-y1)/(y2-y1));
  }

  if (show_progress)
    gimp_progress_update (0.0);

  for (i = 0; i < SOURCE_ROWS; ++i)
    g_free (src[i]);
  g_free (dest);

#ifdef DEBUG_TIME
  gettimeofday (&tv2, NULL);

  sec = tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
  printf ("fix-ca Elapsed time: %.2f\n", sec);
#endif
}

static void
fixca_help (const gchar *help_id, gpointer help_data)
{
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
