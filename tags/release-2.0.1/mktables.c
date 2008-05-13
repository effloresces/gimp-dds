#include <stdlib.h>
#include <stdio.h>

int mul8bit(int a, int b)
{
   int t = a * b + 128;
   return((t + (t >> 8)) >> 8);
}

static void prepare_opt_table(unsigned char *tab,
                              const unsigned char *expand, int size)
{
   int i, mn, mx, bestE, minE, maxE, e;
   
   for(i = 0; i < 256; ++i)
   {
      bestE = 256;
      
      for(mn = 0; mn < size; ++mn)
      {
         for(mx = 0; mx < size; ++mx)
         {
            minE = expand[mn];
            maxE = expand[mx];
            e = abs(maxE + mul8bit(minE - maxE, 0x55) - i);
            
            if(e < bestE)
            {
               tab[i * 2 + 0] = mx;
               tab[i * 2 + 1] = mn;
               bestE = e;
            }
         }
      }
   }
}

int main(void)
{
   FILE *fp;
   int i, v;
   unsigned char expand5[32];
   unsigned char expand6[64];
   unsigned char quantRB[256 + 16];
   unsigned char quantG[256 + 16];
   unsigned char omatch5[256][2];
   unsigned char omatch6[256][2];
   
   fp = fopen("dxt_tables.h", "w");
   fprintf(fp,
           "#ifndef DXT_TABLES_H\n"
           "#define DXT_TABLES_H\n\n");
   
   fprintf(fp,
           "static const unsigned char expand5[32] =\n"
           "{");
   for(i = 0; i < 32; ++i)
   {
      if(i % 8 == 0) fprintf(fp, "\n   ");
      expand5[i] = (i << 3) | (i >> 2);
      fprintf(fp, "0x%02x, ", expand5[i]);
   }
   fprintf(fp, "\n};\n\n");

   fprintf(fp,
           "static const unsigned char expand6[64] =\n"
           "{");
   for(i = 0; i < 64; ++i)
   {
      if(i % 8 == 0) fprintf(fp, "\n   ");
      expand6[i] = (i << 2) | (i >> 4);
      fprintf(fp, "0x%02x, ", expand6[i]);
   }
   fprintf(fp, "\n};\n\n");
   
   for(i = 0; i < 256 + 16; ++i)
   {
      v = i - 8;
      if(v < 0) v = 0;
      if(v > 255) v = 255;
      quantRB[i] = expand5[mul8bit(v, 31)];
      quantG[i] = expand6[mul8bit(v, 63)];
   }
 
   fprintf(fp,
           "static const unsigned char quantRB[256 + 16] =\n"
           "{");
   for(i = 0; i < 256 + 16; ++i)
   {
      if(i % 8 == 0) fprintf(fp, "\n   ");
      fprintf(fp, "0x%02x, ", quantRB[i]);
   }
   fprintf(fp, "\n};\n\n");

   fprintf(fp,
           "static const unsigned char quantG[256 + 16] =\n"
           "{");
   for(i = 0; i < 256 + 16; ++i)
   {
      if(i % 8 == 0) fprintf(fp, "\n   ");
      fprintf(fp, "0x%02x, ", quantG[i]);
   }
   fprintf(fp, "\n};\n\n");
   
   prepare_opt_table(&omatch5[0][0], expand5, 32);
   prepare_opt_table(&omatch6[0][0], expand6, 64);
   
   fprintf(fp,
           "static const unsigned char omatch5[256][2] =\n"
           "{");
   for(i = 0; i < 256; ++i)
   {
      if(i % 4 == 0) fprintf(fp, "\n   ");
      fprintf(fp, "{0x%02x, 0x%02x}, ", omatch5[i][0], omatch5[i][1]);
   }
   fprintf(fp, "\n};\n\n");

   fprintf(fp,
           "static const unsigned char omatch6[256][2] =\n"
           "{");
   for(i = 0; i < 256; ++i)
   {
      if(i % 4 == 0) fprintf(fp, "\n   ");
      fprintf(fp, "{0x%02x, 0x%02x}, ", omatch6[i][0], omatch6[i][1]);
   }
   fprintf(fp, "\n};\n\n");
   
   fprintf(fp, "#endif\n");
   
   fclose(fp);
   
   return(0);
}