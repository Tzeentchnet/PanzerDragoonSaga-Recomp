#include <assert.h>
#include <math.h>
#include "ao.h"
#include "cpuintrf.h"
#include "scsp.h"

static INT32 SCSPDSP_SignExtend(UINT32 value, UINT32 bits)
{
	UINT32 sign=1u<<(bits-1);
	UINT32 mask=(sign<<1)-1;

	value&=mask;
	if(value&sign)
		return -(INT32)((~value+1u)&mask);
	return (INT32)value;
}

static UINT16 PACK(INT32 val)
{
	UINT32 temp;
	UINT32 raw;
	int sign,exponent,k;

	raw=(UINT32)val&0xFFFFFF;
	sign=(raw>>23)&0x1;
	temp=(raw^(raw<<1))&0xFFFFFF;
	exponent = 0;
	for (k=0; k<12; k++)
	{
		if (temp & 0x800000)
			break;
		temp <<= 1;
		exponent += 1;
	}
	if (exponent < 12)
		raw=(raw<<exponent)&0x3FFFFF;
	else
		raw<<=11;
	raw>>=11;
	raw&=0x7FF;
	raw|=(UINT32)sign<<15;
	raw|=(UINT32)exponent<<11;

	return (UINT16)raw;
}

static INT32 UNPACK(UINT16 val)
{
	int sign,exponent,mantissa;
	INT32 uval;

	sign = (val >> 15) & 0x1;
	exponent = (val >> 11) & 0xF;
	mantissa = val & 0x7FF;
	uval = mantissa << 11;
	if (exponent > 11)
	{
		exponent = 11;
		uval |= sign << 22;
	}
	else
		uval |= (sign ^ 1) << 22;
	uval |= sign << 23;
	return SCSPDSP_SignExtend((UINT32)uval,24)>>exponent;
}

void SCSPDSP_Init(struct _SCSPDSP *DSP)
{
	memset(DSP,0,sizeof(struct _SCSPDSP));
	DSP->RBL=0x8000;
	DSP->Stopped=1;
}

static void SCSPDSP_CompleteAccess(struct _SCSPDSP *DSP)
{
	if(DSP->PENDING_READ)
	{
		UINT16 value=0;
		if(DSP->SCSPRAM && DSP->ADDR<DSP->SCSPRAM_LENGTH)
			value=DSP->SCSPRAM[DSP->ADDR];

		if(DSP->PENDING_READ==2)
			DSP->MEMVAL=(INT32)(INT16)value*0x100;
		else
			DSP->MEMVAL=UNPACK(value);
		DSP->PENDING_READ=0;
	}
	else if(DSP->PENDING_WRITE)
	{
		if(DSP->SCSPRAM && DSP->ADDR<DSP->SCSPRAM_LENGTH)
			DSP->SCSPRAM[DSP->ADDR]=DSP->PENDING_VALUE;
		DSP->PENDING_WRITE=0;
	}
}

void SCSPDSP_Step(struct _SCSPDSP *DSP)
{
	int step;
	int steps;

	if(DSP->Stopped)
		return;

	steps=DSP->LastStep<128 ? DSP->LastStep+1 : 128;
	for(step=0; step<steps; ++step)
	{
		UINT16 *IPtr=DSP->MPRO+step*4;
		UINT32 TRA=(IPtr[0]>>8)&0x7F;
		UINT32 TWT=(IPtr[0]>>7)&0x01;
		UINT32 TWA=(IPtr[0]>>0)&0x7F;
		UINT32 XSEL=(IPtr[1]>>15)&0x01;
		UINT32 YSEL=(IPtr[1]>>13)&0x03;
		UINT32 IRA=(IPtr[1]>>6)&0x3F;
		UINT32 IWT=(IPtr[1]>>5)&0x01;
		UINT32 IWA=(IPtr[1]>>0)&0x1F;
		UINT32 TABLE=(IPtr[2]>>15)&0x01;
		UINT32 MWT=(IPtr[2]>>14)&0x01;
		UINT32 MRD=(IPtr[2]>>13)&0x01;
		UINT32 EWT=(IPtr[2]>>12)&0x01;
		UINT32 EWA=(IPtr[2]>>8)&0x0F;
		UINT32 ADRL=(IPtr[2]>>7)&0x01;
		UINT32 FRCL=(IPtr[2]>>6)&0x01;
		UINT32 SHIFT=(IPtr[2]>>4)&0x03;
		UINT32 YRL=(IPtr[2]>>3)&0x01;
		UINT32 NEGB=(IPtr[2]>>2)&0x01;
		UINT32 ZERO=(IPtr[2]>>1)&0x01;
		UINT32 BSEL=(IPtr[2]>>0)&0x01;
		UINT32 NOFL=(IPtr[3]>>8)&1;
		UINT32 COEF=(IPtr[3]>>9)&0x3f;
		UINT32 MASA=(IPtr[3]>>2)&0x1f;
		UINT32 ADREB=(IPtr[3]>>1)&0x1;
		UINT32 NXADR=(IPtr[3]>>0)&0x1;
		INT32 shifted;
		INT32 x;
		INT32 y;
		INT32 b;
		INT32 input;
		INT32 accumulator;
		UINT32 address;
		INT64 v;

		if(IRA<=0x1f)
			DSP->INPUTS=SCSPDSP_SignExtend((UINT32)DSP->MEMS[IRA],24);
		else if(IRA<=0x2F)
			DSP->INPUTS=SCSPDSP_SignExtend((UINT32)DSP->MIXS[IRA-0x20]*16u,24);
		else if(IRA<=0x31)
			DSP->INPUTS=(INT32)DSP->EXTS[IRA-0x30]*0x100;

		input=DSP->INPUTS;
		accumulator=DSP->ACC;
		b=0;
		if(!ZERO)
		{
			b=BSEL ? accumulator : SCSPDSP_SignExtend((UINT32)DSP->TEMP[(TRA+DSP->DEC)&0x7F],24);
			if(NEGB)
				b=-b;
		}

		if(XSEL)
			x=input;
		else
			x=SCSPDSP_SignExtend((UINT32)DSP->TEMP[(TRA+DSP->DEC)&0x7F],24);

		if(YSEL==0)
			y=DSP->FRC_REG;
		else if(YSEL==1)
			y=DSP->COEF[COEF]>>3;
		else if(YSEL==2)
			y=(DSP->Y_REG>>11)&0x1FFF;
		else
			y=(DSP->Y_REG>>4)&0x0FFF;

		if(YRL)
			DSP->Y_REG=input;

		if(SHIFT==0)
		{
			shifted=accumulator;
			if(shifted>0x007FFFFF)
				shifted=0x007FFFFF;
			if(shifted<(-0x00800000))
				shifted=-0x00800000;
		}
		else if(SHIFT==1)
		{
			shifted=accumulator*2;
			if(shifted>0x007FFFFF)
				shifted=0x007FFFFF;
			if(shifted<(-0x00800000))
				shifted=-0x00800000;
		}
		else if(SHIFT==2)
			shifted=SCSPDSP_SignExtend((UINT32)(accumulator*2),24);
		else
			shifted=SCSPDSP_SignExtend((UINT32)accumulator,24);

		v=(((INT64)x*(INT64)SCSPDSP_SignExtend((UINT32)y,13))>>12)+b;
		DSP->ACC=SCSPDSP_SignExtend((UINT32)v,26);

		if(TWT)
			DSP->TEMP[(TWA+DSP->DEC)&0x7F]=shifted;

		if(FRCL)
		{
			if(SHIFT==3)
				DSP->FRC_REG=shifted&0x0FFF;
			else
				DSP->FRC_REG=(shifted>>11)&0x1FFF;
		}

		if(EWT)
			DSP->EFREG[EWA]=(INT16)(shifted>>8);

		if(IWT)
			DSP->MEMS[IWA]=DSP->MEMVAL;

		SCSPDSP_CompleteAccess(DSP);

		address=(UINT32)DSP->MADRS[MASA]+NXADR;
		if(ADREB)
			address=(UINT32)((INT32)address+SCSPDSP_SignExtend(DSP->ADRS_REG,12));
		address&=0xFFFF;
		if(!TABLE)
			address=(address+DSP->DEC)&(DSP->RBL-1);
		DSP->ADDR=(address+(DSP->RBP<<12))&0x7FFFF;

		if(MRD)
			DSP->PENDING_READ=(UINT8)(NOFL ? 2 : 1);
		if(MWT)
		{
			DSP->PENDING_WRITE=1;
			DSP->PENDING_VALUE=NOFL ? (UINT16)(shifted>>8) : PACK(shifted);
		}

		if(ADRL)
		{
			if(SHIFT==3)
				DSP->ADRS_REG=(UINT32)(shifted>>12)&0xFFF;
			else
				DSP->ADRS_REG=(UINT32)(input>>16)&0xFFF;
		}
	}
	--DSP->DEC;
	memset(DSP->MIXS,0,sizeof(DSP->MIXS));
}

void SCSPDSP_SetSample(struct _SCSPDSP *DSP,INT32 sample,int SEL,int MXL)
{
	//DSP->MIXS[SEL]+=sample<<(MXL+1)/*7*/;
	DSP->MIXS[SEL]+=sample;
//  if(MXL)
//      int a=1;
}

void SCSPDSP_Start(struct _SCSPDSP *DSP)
{
	int i;
	DSP->Stopped=0;
	for(i=127; i>=0; --i)
	{
		UINT16 *IPtr=DSP->MPRO+i*4;

		if(IPtr[0]!=0 || IPtr[1]!=0 || IPtr[2]!=0 || IPtr[3]!=0)
			break;
	}
	DSP->LastStep=i+1;

}
