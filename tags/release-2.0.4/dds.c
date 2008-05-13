/*
	DDS GIMP plugin

	Copyright (C) 2004-2008 Shawn Kirst <skirst@insightbb.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "ddsplugin.h"
#include "dds.h"

FILE *errFile;
gchar *prog_name = "dds";
gchar *filename;
gint interactive_dds;

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param,
					 gint *nreturn_vals, GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
	0, 0, query, run
};


DDSWriteVals dds_write_vals =
{
	DDS_COMPRESS_NONE, 0, DDS_SAVE_SELECTED_LAYER, DDS_FORMAT_DEFAULT, -1,
   DDS_COLOR_DEFAULT, 0, DDS_MIPMAP_DEFAULT, 0
};

DDSReadVals dds_read_vals =
{
   1, 1
};

static GimpParamDef load_args[] =
{
   {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
   {GIMP_PDB_STRING, "filename", "The name of the file to load"},
   {GIMP_PDB_STRING, "raw_filename", "The name entered"},
   {GIMP_PDB_INT32, "load_mipmaps", "Load mipmaps if present"}
};
static GimpParamDef load_return_vals[] =
{
   {GIMP_PDB_IMAGE, "image", "Output image"}
};
	
static GimpParamDef save_args[] =
{
   {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
   {GIMP_PDB_IMAGE, "image", "Input image"},
   {GIMP_PDB_DRAWABLE, "drawable", "Drawable to save"},
   {GIMP_PDB_STRING, "filename", "The name of the file to save the image as"},
   {GIMP_PDB_STRING, "raw_filename", "The name entered"},
   {GIMP_PDB_INT32, "compression_format", "Compression format (0 = None, 1 = BC1/DXT1, 2 = BC2/DXT3, 3 = BC3/DXT5, 4 = BC3n/DXT5n, 5 = BC4/ATI1N, 6 = BC5/ATI2N, 7 = Alpha Exponent (DXT5), 8 = YCoCg (DXT5), 9 = YCoCg scaled (DXT5))"},
   {GIMP_PDB_INT32, "generate_mipmaps", "Generate mipmaps"},
   {GIMP_PDB_INT32, "savetype", "How to save the image (0 = selected layer, 1 = cube map, 2 = volume map"},
   {GIMP_PDB_INT32, "format", "Custom pixel format (0 = default, 1 = R5G6B5, 2 = RGBA4, 3 = RGB5A1, 4 = RGB10A2)"},
   {GIMP_PDB_INT32, "transparent_index", "Index of transparent color or -1 to disable (for indexed images only)."},
   {GIMP_PDB_INT32, "color_type", "Color selection algorithm used in DXT compression (0 = default, 1 = distance, 2 = luminance, 3 = inset bounding box)"},
   {GIMP_PDB_INT32, "dither", "Work on dithered color blocks when doing color selection for DXT compression"},
   {GIMP_PDB_INT32, "mipmap_filter", "Filtering to use when generating mipmaps (0 = default, 1 = nearest, 2 = bilinear, 3 = bicubic)"}
};

MAIN()
	
static void query(void)
{
	gimp_install_procedure(LOAD_PROC,
								  "Loads files in DDS image format",
								  "Loads files in DDS image format",
								  "Shawn Kirst",
								  "Shawn Kirst",
								  "2004",
								  "<Load>/DDS image",
								  0,
								  GIMP_PLUGIN,
								  G_N_ELEMENTS(load_args),
                          G_N_ELEMENTS(load_return_vals),
								  load_args, load_return_vals);

   gimp_register_file_handler_mime(LOAD_PROC, "image/dds");
	gimp_register_magic_load_handler(LOAD_PROC,
												"dds",
												"",
												"0,string,DDS");
   
	gimp_install_procedure(SAVE_PROC,
								  "Saves files in DDS image format",
								  "Saves files in DDS image format",
								  "Shawn Kirst",
								  "Shawn Kirst",
								  "2004",
								  "<Save>/DDS image",
								  "INDEXED, GRAY, RGB",
								  GIMP_PLUGIN,
								  G_N_ELEMENTS(save_args), 0,
								  save_args, 0);

   gimp_register_file_handler_mime(SAVE_PROC, "image/dds");
	gimp_register_save_handler(SAVE_PROC,
										"dds",
										"");
}

static void run(const gchar *name, gint nparams, const GimpParam *param,
					 gint *nreturn_vals, GimpParam **return_vals)
{
	static GimpParam values[2];
	GimpRunMode run_mode;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	gint32 imageID;
	gint32 drawableID;
	GimpExportReturn export = GIMP_EXPORT_CANCEL;
	
	run_mode = param[0].data.d_int32;
	
	*nreturn_vals = 1;
	*return_vals = values;
	
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
   
	if(!strcmp(name, LOAD_PROC))
	{
		switch(run_mode)
		{
			case GIMP_RUN_INTERACTIVE:
			   gimp_ui_init("dds", 0);
			   interactive_dds = 1;
            gimp_get_data(LOAD_PROC, &dds_read_vals);
			   break;
			case GIMP_RUN_NONINTERACTIVE:
			   interactive_dds = 0;
            dds_read_vals.show_dialog = 0;
            dds_read_vals.mipmaps = param[3].data.d_int32;
			   if(nparams != G_N_ELEMENTS(load_args))
				   status = GIMP_PDB_CALLING_ERROR;
			   break;
			default:
			   break;
		}
		
		if(status == GIMP_PDB_SUCCESS)
		{
			status = read_dds(param[1].data.d_string, &imageID);
			if(status == GIMP_PDB_SUCCESS && imageID != -1)
			{
				*nreturn_vals = 2;
				values[1].type = GIMP_PDB_IMAGE;
				values[1].data.d_image = imageID;
            if(interactive_dds)
               gimp_set_data(LOAD_PROC, &dds_read_vals, sizeof(dds_read_vals));
			}
			else if(status != GIMP_PDB_CANCEL)
				status = GIMP_PDB_EXECUTION_ERROR;
		}
	}
	else if(!strcmp(name, SAVE_PROC))
	{
		imageID = param[1].data.d_int32;
		drawableID = param[2].data.d_int32;
		
		switch(run_mode)
		{
			case GIMP_RUN_INTERACTIVE:
			case GIMP_RUN_WITH_LAST_VALS:
			   gimp_ui_init("dds", 0);
			   export = gimp_export_image(&imageID, &drawableID, "DDS",
                                       (GIMP_EXPORT_CAN_HANDLE_RGB |
                                        GIMP_EXPORT_CAN_HANDLE_GRAY |
                                        GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                        GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                        GIMP_EXPORT_CAN_HANDLE_LAYERS));
			   if(export == GIMP_EXPORT_CANCEL)
			   {
					values[0].data.d_status = GIMP_PDB_CANCEL;
					return;
				}
			default:
			   break;
		}
		
		switch(run_mode)
		{
			case GIMP_RUN_INTERACTIVE:
			   gimp_get_data(SAVE_PROC, &dds_write_vals);
			   interactive_dds = 1;
			   break;
			case GIMP_RUN_NONINTERACTIVE:
			   interactive_dds = 0;
			   if(nparams != G_N_ELEMENTS(save_args))
				   status = GIMP_PDB_CALLING_ERROR;
			   else
			   {
					dds_write_vals.compression = param[5].data.d_int32;
					dds_write_vals.mipmaps = param[6].data.d_int32;
               dds_write_vals.savetype = param[7].data.d_int32;
               dds_write_vals.format = param[8].data.d_int32;
               dds_write_vals.transindex = param[9].data.d_int32;
               dds_write_vals.color_type = param[10].data.d_int32;
               dds_write_vals.dither = param[11].data.d_int32;
               dds_write_vals.mipmap_filter = param[12].data.d_int32;
               
					if(dds_write_vals.compression < DDS_COMPRESS_NONE ||
						dds_write_vals.compression >= DDS_COMPRESS_MAX)
						status = GIMP_PDB_CALLING_ERROR;
               if(dds_write_vals.savetype < DDS_SAVE_SELECTED_LAYER ||
                  dds_write_vals.savetype >= DDS_SAVE_MAX)
                  status = GIMP_PDB_CALLING_ERROR;
               if(dds_write_vals.format < DDS_FORMAT_DEFAULT ||
                  dds_write_vals.format >= DDS_FORMAT_MAX)
                  status = GIMP_PDB_CALLING_ERROR;
               if(dds_write_vals.color_type < DDS_COLOR_DEFAULT ||
                  dds_write_vals.color_type >= DDS_COLOR_MAX)
                  status = GIMP_PDB_CALLING_ERROR;
               if(dds_write_vals.mipmap_filter < DDS_MIPMAP_DEFAULT ||
                  dds_write_vals.mipmap_filter >= DDS_MIPMAP_MAX)
                  status = GIMP_PDB_CALLING_ERROR;
				}
			   break;
			case GIMP_RUN_WITH_LAST_VALS:
			   gimp_get_data(SAVE_PROC, &dds_write_vals);
			   interactive_dds = 0;
			   break;
			default:
			   break;
		}
		
		if(status == GIMP_PDB_SUCCESS)
		{
			status = write_dds(param[3].data.d_string, imageID, drawableID);
			if(status == GIMP_PDB_SUCCESS)
				gimp_set_data(SAVE_PROC, &dds_write_vals, sizeof(dds_write_vals));
		}
		
		if(export == GIMP_EXPORT_EXPORT)
			gimp_image_delete(imageID);
	}
	else
		status = GIMP_PDB_CALLING_ERROR;

	values[0].data.d_status = status;
}
