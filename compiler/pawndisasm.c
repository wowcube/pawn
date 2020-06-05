/* Pawn disassembler  - crude, but hopefully useful
 *
 *  Copyright (c) CompuPhase, 2007-2020
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: pawndisasm.c 6130 2020-04-29 12:35:51Z thiadmer $
 */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined PAWN_CELL_SIZE
  #define PAWN_CELL_SIZE 64 /* by default, maximum cell size = 64-bit */
#endif
#include "../amx/osdefs.h"
#include "../amx/amx.h"

#if !defined sizearray
  #define sizearray(a)  (sizeof(a) / sizeof((a)[0]))
#endif

static FILE *fpamx;
static AMX_HEADER amxhdr;
static int pc_cellsize;
static ucell cellmask;

typedef cell (*OPCODE_PROC)(FILE *ftxt,const cell *params,cell opcode,cell cip);

static cell parm0(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell parm1(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell parm2(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell parmx(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell parm1_p(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell parmx_p(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell do_proc(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell do_call(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell do_jump(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell do_switch(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell casetbl(FILE *ftxt,const cell *params,cell opcode,cell cip);
static cell casetbl_ovl(FILE *ftxt,const cell *params,cell opcode,cell cip);


typedef struct tagLABEL {
  struct tagLABEL *next;
  int seq;
  cell address;
} LABEL;

static LABEL labelroot = { NULL };


typedef struct tagOPCODE {
  cell opcode;
  char *name;
  OPCODE_PROC func;
} OPCODE;

static OPCODE opcodelist[] = {
  {  0, "nop",         parm0 },
  {  1, "load.pri",    parm1 },
  {  2, "load.alt",    parm1 },
  {  3, "load.s.pri",  parm1 },
  {  4, "load.s.alt",  parm1 },
  {  5, "lref.s.pri",  parm1 },
  {  6, "lref.s.alt",  parm1 },
  {  7, "load.i",      parm0 },
  {  8, "lodb.i",      parm1 },
  {  9, "const.pri",   parm1 },
  { 10, "const.alt",   parm1 },
  { 11, "addr.pri",    parm1 },
  { 12, "addr.alt",    parm1 },
  { 13, "stor",        parm1 },
  { 14, "stor.s",      parm1 },
  { 15, "sref.s",      parm1 },
  { 16, "stor.i",      parm0 },
  { 17, "strb.i",      parm1 },
  { 18, "align.pri",   parm1 },
  { 19, "lctrl",       parm1 },
  { 20, "sctrl",       parm1 },
  { 21, "xchg",        parm0 },
  { 22, "push.pri",    parm0 },
  { 23, "push.alt",    parm0 },
  { 24, "pushr.pri",   parm0 },
  { 25, "pop.pri",     parm0 },
  { 26, "pop.alt",     parm0 },
  { 27, "pick",        parm1 },
  { 28, "stack",       parm1 },
  { 29, "heap",        parm1 },
  { 30, "proc",        do_proc },
  { 31, "ret",         parm0 },
  { 32, "retn",        parm0 },
  { 33, "call",        do_call },
  { 34, "jump",        do_jump },
  { 35, "jzer",        do_jump },
  { 36, "jnz",         do_jump },
  { 37, "shl",         parm0 },
  { 38, "shr",         parm0 },
  { 39, "sshr",        parm0 },
  { 40, "shl.c.pri",   parm1 },
  { 41, "shl.c.alt",   parm1 },
  { 42, "smul",        parm0 },
  { 43, "sdiv",        parm0 },
  { 44, "add",         parm0 },
  { 45, "sub",         parm0 },
  { 46, "and",         parm0 },
  { 47, "or",          parm0 },
  { 48, "xor",         parm0 },
  { 49, "not",         parm0 },
  { 50, "neg",         parm0 },
  { 51, "invert",      parm0 },
  { 52, "eq",          parm0 },
  { 53, "neq",         parm0 },
  { 54, "sless",       parm0 },
  { 55, "sleq",        parm0 },
  { 56, "sgrtr",       parm0 },
  { 57, "sgeq",        parm0 },
  { 58, "inc.pri",     parm0 },
  { 59, "inc.alt",     parm0 },
  { 60, "inc.i",       parm0 },
  { 61, "dec.pri",     parm0 },
  { 62, "dec.alt",     parm0 },
  { 63, "dec.i",       parm0 },
  { 64, "movs",        parm1 },
  { 65, "cmps",        parm1 },
  { 66, "fill",        parm1 },
  { 67, "halt",        parm1 },
  { 68, "bounds",      parm1 },
  { 69, "sysreq",      parm1 },
  { 70, "switch",      do_switch },
  { 71, "swap.pri",    parm0 },
  { 72, "swap.alt",    parm0 },
  { 73, "break",       parm0 },
  { 74, "casetbl",     casetbl },
  { 75, "sysreq.d",    parm1 }, /* not generated by the compiler */
  { 76, "sysreq.nd",   parm2 }, /* not generated by the compiler */
  { 77, "call.ovl",    parm1 },
  { 78, "retn.ovl",    parm0 },
  { 79, "switch.ovl",  do_switch },
  { 80, "casetbl.ovl", casetbl_ovl },
  { 81, "lidx",        parm0 },
  { 82, "lidx.b",      parm1 },
  { 83, "idxaddr",     parm0 },
  { 84, "idxaddr.b",   parm1 },
  { 85, "push.c",      parm1 },
  { 86, "push",        parm1 },
  { 87, "push.s",      parm1 },
  { 88, "push.adr",    parm1 },
  { 89, "pushr.c",     parm1 },
  { 90, "pushr.s",     parm1 },
  { 91, "pushr.adr",   parm1 },
  { 92, "jeq",         do_jump },
  { 93, "jneq",        do_jump },
  { 94, "jsless",      do_jump },
  { 95, "jsleq",       do_jump },
  { 96, "jsgrtr",      do_jump },
  { 97, "jsgeq",       do_jump },
  { 98, "sdiv.inv",    parm0 },
  { 99, "sub.inv",     parm0 },
  {100, "add.c",       parm1 },
  {101, "smul.c",      parm1 },
  {102, "zero.pri",    parm0 },
  {103, "zero.alt",    parm0 },
  {104, "zero",        parm1 },
  {105, "zero.s",      parm1 },
  {106, "eq.c.pri",    parm1 },
  {107, "eq.c.alt",    parm1 },
  {108, "inc",         parm1 },
  {109, "inc.s",       parm1 },
  {110, "dec",         parm1 },
  {111, "dec.s",       parm1 },
  {112, "sysreq.n",    parm2 },
  {113, "pushm.c",     parmx },
  {114, "pushm",       parmx },
  {115, "pushm.s",     parmx },
  {116, "pushm.adr",   parmx },
  {117, "pushrm.c",    parmx },
  {118, "pushrm.s",    parmx },
  {119, "pushrm.adr",  parmx },
  {120, "load2",       parm2 },
  {121, "load2.s",     parm2 },
  {122, "const",       parm2 },
  {123, "const.s",     parm2 },
  {124, "load.p.pri",  parm1_p },
  {125, "load.p.alt",  parm1_p },
  {126, "load.p.s.pri",parm1_p },
  {127, "load.p.s.alt",parm1_p },
  {128, "lref.p.s.pri",parm1_p },
  {129, "lref.p.s.alt",parm1_p },
  {130, "lodb.p.i",    parm1_p },
  {131, "const.p.pri", parm1_p },
  {132, "const.p.alt", parm1_p },
  {133, "addr.p.pri",  parm1_p },
  {134, "addr.p.alt",  parm1_p },
  {135, "stor.p",      parm1_p },
  {136, "stor.p.s",    parm1_p },
  {137, "sref.p.s",    parm1_p },
  {138, "strb.p.i",    parm1_p },
  {139, "lidx.p.b",    parm1_p },
  {140, "idxaddr.p.b", parm1_p },
  {141, "align.p.pri", parm1_p },
  {142, "push.p.c",    parm1_p },
  {143, "push.p",      parm1_p },
  {144, "push.p.s",    parm1_p },
  {145, "push.p.adr",  parm1_p },
  {146, "pushr.p.c",   parm1_p },
  {147, "pushr.p.s",   parm1_p },
  {148, "pushr.p.adr", parm1_p },
  {149, "pushm.p.c",   parmx_p },
  {150, "pushm.p",     parmx_p },
  {151, "pushm.p.s",   parmx_p },
  {152, "pushm.p.adr", parmx_p },
  {153, "pushrm.p.c",  parmx_p },
  {154, "pushrm.p.s",  parmx_p },
  {155, "pushrm.p.adr",parmx_p },
  {156, "stack.p",     parm1_p },
  {157, "heap.p",      parm1_p },
  {158, "shl.p.c.pri", parm1_p },
  {159, "shl.p.c.alt", parm1_p },
  {160, "add.p.c",     parm1_p },
  {161, "smul.p.c",    parm1_p },
  {162, "zero.p",      parm1_p },
  {163, "zero.p.s",    parm1_p },
  {164, "eq.p.c.pri",  parm1_p },
  {165, "eq.p.c.alt",  parm1_p },
  {166, "inc.p",       parm1_p },
  {167, "inc.p.s",     parm1_p },
  {168, "dec.p",       parm1_p },
  {169, "dec.p.s",     parm1_p },
  {170, "movs.p",      parm1_p },
  {171, "cmps.p",      parm1_p },
  {172, "fill.p",      parm1_p },
  {173, "halt.p",      parm1_p },
  {174, "bounds.p",    parm1_p },
};


static void label_add(cell address)
{
  LABEL *item, *last;
  int seq=0;

  /* find the last existing label entry */
  for (last=&labelroot; last->next!=NULL; last=last->next)
    seq++;
  if ((item=malloc(sizeof(LABEL)))!=NULL) {
    item->seq=seq;
    item->address=address;
    item->next=NULL;
    last->next=item;
  }
}

static const char *label_get(cell address)
{
  static char str[10];
  LABEL *item;

  for (item=labelroot.next; item!=NULL && item->address!=address; item=item->next)
    /* nothing */;
  if (item!=NULL)
    sprintf(str,"L%d:",item->seq);
  else
    str[0]='\0';
  return str;
}

static void label_deletall(void)
{
  LABEL *item;
  while (labelroot.next!=NULL) {
    item=labelroot.next;
    labelroot.next=item->next;
    free(item);
  }
}

static void print_opcode(FILE *ftxt,cell opcode,cell cip)
{
  assert(ftxt!=NULL);
  assert(opcodelist[(int)(opcode & 0x0000ffff)].opcode==(opcode & 0x0000ffff));
  fprintf(ftxt,"%08lx  %-6s%s ",(long)cip,label_get(cip),opcodelist[(int)(opcode & 0x0000ffff)].name);
}

static void print_param(FILE *ftxt,cell param,int eol)
{
  assert(ftxt!=NULL);
  param &= cellmask;
  switch (pc_cellsize) {
  case 2:
    fprintf(ftxt,"%0" PRIx16,(uint16_t)param);
    break;
  case 4:
    fprintf(ftxt,"%0" PRIx32,(uint32_t)param);
    break;
  case 8:
    fprintf(ftxt,"%0" PRIx64,(uint64_t)param);
    break;
  default:
    assert(0);
  } /* switch */
  if (eol)
    fprintf(ftxt,"\n");
  else
    fprintf(ftxt," ");
}

static cell get_param(const cell *params, int index)
{
  cell value;
  switch (pc_cellsize) {
  case 2:
    value=(cell)*((uint16_t*)params+index);
    break;
  case 4:
    value=(cell)*((uint32_t*)params+index);
    break;
  case 8:
    value=(cell)*((uint64_t*)params+index);
    break;
  default:
    assert(0);
  } /* switch */
  return value;
}

static cell parm0(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  (void)params;
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    fprintf(ftxt,"\n");
  }
  return 1;
}

cell parm1(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0),1);
  }
  return 2;
}

static cell parm2(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0),0);
    print_param(ftxt,get_param(params,1),1);
  }
  return 3;
}

static cell parmx(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  int count=(int)get_param(params,0);
  if (ftxt!=NULL) {
    int idx;
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,count,0);
    for (idx=0; idx<count-1; idx++)
      print_param(ftxt,get_param(params,idx+1),0);
    print_param(ftxt,get_param(params,idx+1),1);
  }
  return count+2;
}

static cell parm1_p(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  (void)params;
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    fprintf(ftxt,"%08lx\n",(long)((ucell)opcode>>(pc_cellsize*4)));
  }
  return 1;
}

static cell parmx_p(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  int count=(int)((ucell)opcode>>(pc_cellsize*4));
  if (ftxt!=NULL) {
    int idx;
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,count,0);
    for (idx=0; idx<count-1; idx++)
      print_param(ftxt,get_param(params,idx),0);
    print_param(ftxt,get_param(params,idx),1);
  }
  return count+1;
}

static cell do_proc(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  AMX_FUNCSTUB func;

  (void)params;
  if (ftxt!=NULL) {
    int nameoffset;
    char name[sNAMEMAX+1];

    fprintf(ftxt,"\n");

    /* find the procedure in the table (only works for a public function) */
    nameoffset=-1;
    name[0]='\0';
    if ((amxhdr.flags & AMX_FLAG_OVERLAY)!=0) {
      //??? first find the address in the overlay table
      //??? find the overlay index in the public function table
    } else {
      /* find the address in the public function table */
      int idx;
      int numpublics=(amxhdr.natives-amxhdr.publics)/sizeof(AMX_FUNCSTUB);
      fseek(fpamx,amxhdr.publics,SEEK_SET);
      for (idx=0; idx<numpublics && nameoffset<0; idx++) {
        fread(&func,sizeof func,1,fpamx);
        if (func.address==cip)
          nameoffset=func.nameofs;
      } /* for */
    } /* if */
    if (nameoffset>=0) {
      fseek(fpamx,nameoffset,SEEK_SET);
      fread(name,1,sNAMEMAX+1,fpamx);
    } /* if */

    print_opcode(ftxt,opcode,cip);
    if (strlen(name)>0)
      fprintf(ftxt,"\t; %s",name);
    fprintf(ftxt,"\n");
  }
  return 1;
}

static cell do_call(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0)+cip,1);
  }
  return 2;
}

static cell do_jump(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  if (ftxt!=NULL) {
    LABEL *item;
    cell address=get_param(params,0)+cip;
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,address,0);
    for (item=labelroot.next; item!=NULL && item->address!=address; item=item->next)
      /* nothing */;
    if (item!=NULL)
      fprintf(ftxt,"\t; L%d",item->seq);
    fprintf(ftxt,"\n");
  }
  return 2;
}

static cell do_switch(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  if (ftxt!=NULL) {
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0)+cip,1);
  }
  return 2;
}

static cell casetbl(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  cell num=get_param(params,0)+1;
  if (ftxt!=NULL) {
    int idx;
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0),0);
    print_param(ftxt,get_param(params,1)+cip+pc_cellsize,1);
    for (idx=1; idx<num; idx++) {
      fprintf(ftxt,"                  ");
      print_param(ftxt,get_param(params,2*idx),0);
      print_param(ftxt,get_param(params,2*idx+1)+cip+(2*idx+1)*pc_cellsize,1);
    } /* for */
  }
  return 2*num+1;
}

static cell casetbl_ovl(FILE *ftxt,const cell *params,cell opcode,cell cip)
{
  cell num=get_param(params,0)+1;
  if (ftxt!=NULL) {
    int idx;
    print_opcode(ftxt,opcode,cip);
    print_param(ftxt,get_param(params,0),0);
    print_param(ftxt,get_param(params,1),1);
    for (idx=1; idx<num; idx++) {
      fprintf(ftxt,"                  ");
      print_param(ftxt,get_param(params,2*idx),0);
      print_param(ftxt,get_param(params,2*idx+1),1);
    } /* for */
  }
  return 2*num+1;
}

static void addchars(char *str,int64_t value,int pos)
{
  int i;
  str+=pos*pc_cellsize;
  for (i=0; i<pc_cellsize; i++) {
    int v=(value >> 8*(pc_cellsize-1)) & 0xff;
    value <<= 8;
    *str++= (v>=32) ? (char)v : ' ';
  } /* for */
  *str='\0';
}

#if (defined _MSC_VER || defined __GNUC__) && !defined __clang__
/* Copy src to string dst of size siz.
 * At most siz-1 characters * will be copied. Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred                        .
 *                                                                                   .
 *  Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>, MIT license.                                                                                  .
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}
#endif

int main(int argc,char *argv[])
{
  char name[_MAX_PATH];
  FILE *fplist;
  int codesize,count;
  unsigned char *code,*cip;
  OPCODE_PROC func;
  cell opc;

  if (argc<2 || argc>3) {
    printf("Usage: pawndisasm <input> [output]\n");
    return 1;
  } /* if */
  if (argc==2) {
    char *ptr;
    strlcpy(name,argv[1],sizearray(name));
    if ((ptr=strrchr(name,'.'))!=NULL && strpbrk(ptr,"\\/:")==NULL)
      *ptr='\0';          /* erase existing extension */
    strcat(name,".lst");  /* append new extension */
  } else {
    strlcpy(name,argv[2],sizearray(name));
  } /* if */
  if ((fpamx=fopen(argv[1],"rb"))==NULL) {
    printf("Unable to open input file \"%s\"\n",argv[1]);
    return 1;
  } /* if */
  if ((fplist=fopen(name,"wt"))==NULL) {
    printf("Unable to create output file \"%s\"\n",name);
    return 1;
  } /* if */

  /* load header */
  fread(&amxhdr,sizeof amxhdr,1,fpamx);
  if (amxhdr.magic==AMX_MAGIC_16) {
    pc_cellsize=2;
    cellmask=0xffffU;
  } else if (amxhdr.magic==AMX_MAGIC_32) {
    pc_cellsize=4;
    cellmask=0xffffffffLU;
  } else if (amxhdr.magic==AMX_MAGIC_64) {
    pc_cellsize=4;
    cellmask=(ucell)0xffffffffffffffffLL;  /* suffix "LLU" is unsupported on Microsoft Visual C/C++ */
  } else {
    printf("Not a valid AMX file\n");
    fclose(fplist);
    return 1;
  } /* if */
  if (amxhdr.flags & AMX_FLAG_CRYPT) {
    printf("This is an encrypted script. Encrypted scripts cannot be disassembled.\n");
    fclose(fplist);
    return 1;
  } /* if */

  codesize=amxhdr.hea-amxhdr.cod; /* size for both code and data */
  fprintf(fplist,";File version: %d\n",amxhdr.file_version);
  fprintf(fplist,";Cell size: %d bits\n",8*pc_cellsize);
  fprintf(fplist,";Flags:        ");
  if ((amxhdr.flags & AMX_FLAG_SLEEP)!=0)
    fprintf(fplist,"sleep(re-entrant) ");
  if ((amxhdr.flags & AMX_FLAG_OVERLAY)!=0)
    fprintf(fplist,"overlays ");
  if ((amxhdr.flags & AMX_FLAG_DEBUG)!=0)
    fprintf(fplist,"debug-info ");
  fprintf(fplist,"\n\n");
  /* load the code block */
  if ((code=(unsigned char*)malloc(codesize))==NULL) {
    printf("Insufficient memory: need %d bytes\n",codesize);
    fclose(fplist);
    return 1;
  } /* if */

  /* read the file */
  fseek(fpamx,amxhdr.cod,SEEK_SET);
  fread(code,1,codesize,fpamx);

  /* do a first run through the code to get jump targets (for labels) */
  cip=code;
  codesize=amxhdr.dat-amxhdr.cod;
  while (cip-code<codesize) {
    switch (pc_cellsize) {
    case 2:
      opc=(cell)*(uint16_t*)cip;
      func=opcodelist[(int)(opc&0xff)].func;
      break;
    case 4:
      opc=(cell)*(uint32_t*)cip;
      func=opcodelist[(int)(opc&0xffff)].func;
      break;
    case 8:
      opc=(cell)*(uint64_t*)cip;
      func=opcodelist[(int)(opc&0xffffffffLU)].func;
      break;
    default:
      assert(0);
    } /* switch */
    opc=func(NULL,(cell*)(cip+pc_cellsize),opc,(cell)(cip-code));
    cip+=pc_cellsize*opc;
  } /* while */

  /* browse through the code */
  cip=code;
  codesize=amxhdr.dat-amxhdr.cod;
  while (cip-code<codesize) {
    switch (pc_cellsize) {
    case 2:
      opc=(cell)*(uint16_t*)cip;
      func=opcodelist[(int)(opc&0xff)].func;
      break;
    case 4:
      opc=(cell)*(uint32_t*)cip;
      func=opcodelist[(int)(opc&0xffff)].func;
      break;
    case 8:
      opc=(cell)*(uint64_t*)cip;
      func=opcodelist[(int)(opc&0xffffffffLU)].func;
      break;
    default:
      assert(0);
    } /* switch */
    if (func==do_jump)
      label_add(get_param((cell*)(cip+pc_cellsize),0)+(cell)(cip-code));
    opc=func(fplist,(cell*)(cip+pc_cellsize),opc,(cell)(cip-code));
    cip+=pc_cellsize*opc;
  } /* while */

  /* dump the data section too */
  fprintf(fplist,"\n\n;DATA");
  cip=code+(amxhdr.dat-amxhdr.cod);
  codesize=amxhdr.hea-amxhdr.cod;
  count=0;
  name[0]='\0';
  while (cip-code<codesize) {
    if (count==0) {
      if (strlen(name)>0) {
        fprintf(fplist," %s",name);
        name[0]='\0';
      } /* if */
      fprintf(fplist,"\n%08lx  ",(long)((cip-code)-(amxhdr.dat-amxhdr.cod)));
    } /* if */
    switch (pc_cellsize) {
    case 2:
      fprintf(fplist,"%0" PRIx16 " ",*(uint16_t*)cip);
      addchars(name,*(short*)cip,count);
      break;
    case 4:
      fprintf(fplist,"%0" PRIx32 " ",*(uint32_t*)cip);
      addchars(name,*(long*)cip,count);
      break;
    case 8:
      fprintf(fplist,"%0" PRIx64 " ",*(uint64_t*)cip);
      addchars(name,*(int64_t*)cip,count);
      break;
    default:
      assert(0);
    } /* case */
    count=(count+1) % 4;
    cip+=pc_cellsize;
  } /* while */
  if (strlen(name)>0)
    fprintf(fplist," %s",name);

  free(code);
  label_deletall();
  fclose(fpamx);
  fclose(fplist);
  return 0;
}

