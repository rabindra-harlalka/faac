/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: aacquant.h,v 1.9 2003/10/12 16:43:39 knik Exp $
 */

#ifndef AACQUANT_H
#define AACQUANT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "coder.h"

#define IXMAX_VAL 8191
#define PRECALC_SIZE (IXMAX_VAL+2)
#define LARGE_BITS 100000
#define SF_OFFSET 100

#define POW20(x)  pow(2.0,((double)x)*.25)
#define IPOW20(x)  pow(2.0,-((double)x)*.1875)

typedef struct
  {
    double *pow43;
    double quality;
    int max_cbl;
    int max_cbs;
  } AACQuantCfg;

#include "quantize.h"

enum {DEFQUAL = 100, MAXQUAL = 5000, MAXQUALADTS = MAXQUAL, MINQUAL = 10};

void AACQuantizeInit(CoderInfo *coderInfo, unsigned int numChannels,
		     AACQuantCfg *aacquantCfg);
void AACQuantizeEnd(CoderInfo *coderInfo, unsigned int numChannels,
		    AACQuantCfg *aacquantCfg);

int AACQuantize(CoderInfo *coderInfo,
                int *cb_width,
                int num_cb,
                double *xr,
		AACQuantCfg *aacquantcfg);

int SortForGrouping(CoderInfo* coderInfo,
		    int *sfb_width_table,
		    double *xr);
void CalcAvgEnrg(CoderInfo *coderInfo,
		 const double *xr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AACQUANT_H */
