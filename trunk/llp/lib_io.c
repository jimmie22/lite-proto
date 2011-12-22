#include "lp_conf.h"
#include "lib_lp.h"
#include "lib_io.h"
#include "lib_mes.h"

static int sl_W32(slice* out, llp_uint32 num);
static int sl_W64(slice* out, llp_uint64 num);
static int sl_relloc(slice* sl);

#define  EXT_SLI_LENS   256
#define  Rtag_id(tag)				((tag)>>3)
#define  Rtag_type(tag)				((byte)((tag)&0x07))
#define  sl_lens(out)				(((out)->sp)-((out)->b_sp))
#define  sl_is_end(in)				( (sl_emp(in)==0)?(1):(0) )
#define  sl_check_lens(in, lens)	do{ if( sl_emp(in)<(lens) ) return LP_FAIL; }while(0)
#define  sl_Wbyte(out, number)		do{		\
										if(sl_emp(out)<sizeof(byte))	\
											check_fail(sl_relloc(out), LP_FAIL);		\
										*((out)->sp++) = (number);	\
									}while(0)

#define sl_Wnum(out, num)			do{		\
										int i=0;	\
										for(i=0; i<((sizeof(num)<<3)/7); i++)	\
										{	\
											if(num >= 0x80)	\
											{	\
												sl_Wbyte(out, (byte)(num | 0x80));	\
												num >>= 7;	\
											}	\
											else	\
												break;	\
										}	\
										sl_Wbyte(out, ((byte)num) );	\
									}while(0)


#define  sl_Rnum(in, num_p)			do{	\
										int i=0;	\
										for(i=0; i<((sizeof(*(num_p))<<3)/7+1) && sl_is_end(in)==0; i++)	\
										{	\
											(*(num_p)) |= (((*(in->sp)) & 0x7F)<<(7*i));	\
											if( ((*(in->sp++))&(0x80))==0 )	\
												return LP_TRUE;		\
										}		\
										*(num_p) = 0;	\
										return LP_FAIL;	\
									}while(0)


static int sl_relloc(slice* sl)
{
	byte* re_p = (byte*)realloc(sl->b_sp, sl->sp_size+EXT_SLI_LENS);
	check_null(re_p, LP_FAIL);
	sl->sp = re_p+(sl->sp-sl->b_sp);
	sl->b_sp = re_p;
	sl->sp_size += EXT_SLI_LENS;
	return LP_TRUE;
}

int sl_Rstr(slice* sl, char** out)
{
	check_sl(sl);
	*out = (char*)(sl->sp);
	sl->sp += (strlen(*out)+1);
	check_sl(sl);
	
	return LP_TRUE;
}	

int sl_Rlens(slice* sl, unsigned int* out)
{
	check_sl(sl);
	*out = *((unsigned int*)(sl->sp));
	sl->sp += sizeof(unsigned int);
	sl->sp += (*out);
	check_sl(sl);
	sl->sp -= (*out);
	
	return LP_TRUE;
}

int sl_Rbyte(slice* sl, byte* out)
{
	check_sl(sl);
	*out = *(sl->sp);
	sl->sp += 1;
	check_sl(sl);
	
	return LP_TRUE;
}

int  sl_Ruint(slice* sl, size_t* out)
{
	check_sl(sl);
	*out = *((size_t*)(sl->sp));
	sl->sp += sizeof(size_t);
	check_sl(sl);
	
	return LP_TRUE;
}


int sl_Wtag(slice* out, e_ot ot, unsigned int id)
{
	return sl_W32(out, ((id<<3)|((unsigned int)ot)) );
}

static int sl_W32(slice* out, llp_uint32 num)
{
	sl_Wnum(out, num);
	return LP_TRUE;
}

static int sl_R32(slice* in, llp_uint32* num_p)
{
	*num_p=  0;
	sl_Rnum(in, num_p);
}


int sl_Wint32(slice* out, llp_int32 num)
{
	return  sl_W32(out, *((llp_uint32*)(&num)) );
}

int sl_Rint32(slice* in, llp_int32* num_p)
{
	return sl_R32(in, (llp_uint32*)num_p);
}

int sl_Wfloat32(slice* out, llp_float32 num)
{
	return sl_W32(out, *((llp_uint32*)(&num)) );
}

int sl_Rfloat32(slice* in, llp_float32* num_p)
{
	return sl_R32(in, (llp_uint32*)num_p); 
}

static int sl_W64(slice* out, llp_uint64 num)
{
	sl_Wnum(out, num);
	return LP_TRUE;
}

static int sl_R64(slice* in, llp_uint64* num_p)
{
	sl_Rnum(in, num_p);
}

int sl_Wint64(slice* out, llp_int64 num)
{
	return sl_W64(out, *((llp_uint64*)&(num)) );
}

int sl_Rint64(slice* in, llp_int64* num_p)
{
	return sl_R64(in, (llp_uint64*)num_p);
}

int sl_Wfloat64(slice* out, llp_float64 num)
{
	return sl_W64(out, *((llp_uint64*)&(num)) );
}

int sl_Rfloat64(slice* in, llp_float64* num_p)
{
	return sl_R64(in, (llp_uint64*)num_p);
}

int sl_Wstring(slice* out, char* str)
{
	while(*str)
	{
		sl_Wbyte(out, (byte)(*str));
		str++;
	}
	sl_Wbyte(out, 0);
	return LP_TRUE;
}

int sl_Rstring(slice* in, char** str_p)
{
	size_t len = strlen((char*)in->sp)+1;
	sl_check_lens(in, len);
	*str_p = (char*)in->sp;
	in->sp += len;

	return LP_TRUE;
}

int sl_Wmessage(slice* out, llp_mes* lms)
{
	int i=0;
	check_fail(sl_W32(out, (llp_uint32)sl_lens(&lms->sio)), LP_FAIL);			// write lens
	for(i=0; i<sl_lens(&lms->sio); i++)										// write message body
		sl_Wbyte(out, lms->sio.b_sp[i]);

	return LP_TRUE;
}

int  llp_out_open(slice* out)
{
	check_null(out, LP_FAIL);
	if(out->sp_size == 0)
	{
		out->sp = (byte*)malloc(EXT_SLI_LENS*sizeof(byte));
		out->b_sp = out->sp;
		out->sp_size = EXT_SLI_LENS;
	}
	else
		llp_out_clr(out);

	return LP_TRUE;
}

int llp_out_close(slice* out)
{
	check_null(out, LP_FAIL);
	if(out->b_sp)
		free(out->b_sp);
	memset(out, 0, sizeof(*out));

	return LP_TRUE;
}

int llp_out_clr(slice* out)
{
	check_null(out, LP_FAIL);
	out->sp = out->b_sp;
	return LP_TRUE;
}

// API
int llp_in_message(slice* in, llp_mes* lms)
{
	size_t Ri = 0;
	byte   tt = 0;
	llp_uint32 Rtag =0;
	check_null(lms, LP_FAIL);
	check_null(in, LP_FAIL);
	check_null(in->sp, LP_FAIL);

	while(sl_is_end(in)==0)
	{
		check_fail(sl_R32(in, &Rtag), LP_FAIL);		// read tag
		
		if( (Ri=Rtag_id(Rtag))>=lms->filed_lens )		// check ID is true
			return LP_FAIL;
		
		// check tag type is true
		switch(tt=(byte)tag_type(lms->d_mes->message_tfl[Ri].tag))
		{
		case lpt_int32:
			{
				llp_int32 temp = 0;
				if(Rtag_type(Rtag)!= o_num)
					return LP_FAIL;
				check_fail(sl_Rint32(in, &temp), LP_FAIL);
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(&temp)), LP_FAIL);
			}
			break;
		case lpt_int64:
			{
				llp_int64 temp =0;
				if(Rtag_type(Rtag)!= o_num)
					return LP_FAIL;
				check_fail(sl_Rint64(in, &temp), LP_FAIL);
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(&temp)), LP_FAIL);
			}
			break;
		case lpt_float32:
			{
				llp_float32 temp = 0.0;
				if(Rtag_type(Rtag)!= o_num)
					return LP_FAIL;
				check_fail(sl_Rfloat32(in, &temp), LP_FAIL);
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(&temp)), LP_FAIL);
			}
			break;
		case lpt_float64:
			{
				llp_float64 temp = 0.0;
				if(Rtag_type(Rtag)!= o_num)
					return LP_FAIL;
				check_fail(sl_Rfloat64(in, &temp), LP_FAIL);
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(&temp)), LP_FAIL);
			}
			break;
		case  lpt_string:
			{
				char* temp = NULL;
				if(Rtag_type(Rtag)!= o_str)
					return LP_FAIL;
				check_fail(sl_Rstring(in, &temp), LP_FAIL);
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(temp)), LP_FAIL);
			}
			break;
		case lpt_message:
			{
				slice st = {0};
				llp_mes* temp = NULL;
				if(Rtag_type(Rtag)!= o_mes)
					return LP_FAIL;
				check_fail(sl_R32(in, (llp_uint32*)(&st.sp_size)), LP_FAIL);		// read message lens
				st.sp = in->sp;
				st.b_sp = st.sp;
				check_fail(_llp_Wmes(lms, Ri, tt, (void*)(&temp)), LP_FAIL);
				check_fail(llp_in_message(&st, temp), LP_FAIL);
				in->sp += st.sp_size;
			}
			break;
		default:
			return LP_FAIL;
		}
	}

	return LP_TRUE;
}

static int _llp_out_message(llp_mes* lms)
{
	unsigned int inx =0;
	size_t i=0;
	check_null(lms, LP_FAIL);
	llp_out_open(&lms->sio);			// open out buff
//	sl_W32(lms->d_mes->message_id);		// if write id?
	
	for(i=0; i<lms->filed_lens; i++)
	{
		switch(tag_type(lms->d_mes->message_tfl[i].tag))
		{
		case lpt_int32:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_num, i);
					sl_Wint32(&lms->sio, lv->lp_int32);
				}
			}
			break;
		case lpt_int64:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_num, i);
					sl_Wint64(&lms->sio, lv->lp_int64);
				}
			}
			break;
		case lpt_float32:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_num, i);
					sl_Wfloat32(&lms->sio, lv->lp_float32);
				}
			}
			break;
		case  lpt_float64:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_num, i);
					sl_Wfloat64(&lms->sio, lv->lp_float64);
				}
			}
			break;
		case lpt_string:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_str, i);
					sl_Wstring(&lms->sio, lv->lp_str);
				}
			}
			break;
		case lpt_message:
			{
				for(inx=0; inx<lms->filed_al[i].lens; inx++)
				{
					llp_value* lv = lib_array_inx(&lms->filed_al[i], inx);
					sl_Wtag(&lms->sio, o_mes, i);
					check_fail(_llp_out_message(lv->lp_mes), LP_FAIL);
					sl_Wmessage(&lms->sio, lv->lp_mes);
					llp_out_close(&lv->lp_mes->sio);			// close the message slice
				}
			}
			break;
		default:
			return LP_FAIL;
		}
	}
	return LP_TRUE;
}

// api
slice* llp_out_message(llp_mes* lms)
{
	check_fail(_llp_out_message(lms), NULL);
	
	lms->sret.b_sp = lms->sio.b_sp;
	lms->sret.sp = lms->sio.b_sp;
	lms->sret.sp_size = lms->sio.sp - lms->sio.b_sp;

	return &lms->sret;
}