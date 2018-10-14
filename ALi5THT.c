/*----------------------------------------------------------------------------
ALi Aladdin V ACPI enabler for Throttle - G. Broers 2014, thanks to Chris Giese.
Throttle C port and integration - Braden "Blzut3" Obrzut 2018

Text below from original PCI.C, where this enabler is based on. 

--------------------------------------------------------------------

PCI demo for DJGPP, Turbo C, or Watcom C, using I/O ports
(configuration mechanism 1)
Chris Giese     <geezer@execpc.com>     http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.

March 22, 2012:
- replaced pci_t struct with 16-bit 'bdf' value
- removed "_config" from function names to shorten them
- pci_read_(blah) now returns value read instead of useless error code
- pci_write_(blah) no longer returns useless error code

August 27, 2011:
- changed device validation code
- stylistic changes to pci_iterate()

July 23, 2009: modified to display device API

April 25, 2007: initial release
----------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>

#if defined(__DJGPP__)
#include <dos.h> /* inport[b|w|l](), outport[b|w|l]() */

#elif defined(__WATCOMC__)
#include <conio.h>

#define inportb(P)      inp(P)
#define inportw(P)      inpw(P)
#define outportb(P,V)   outp(P,V)
#define outportw(P,V)   outpw(P,V)
#if defined(__386__)
#define inportl(P)      inpd(P)
#define outportl(P,V)   outpd(P,V)
/* else [in|out]portl() defined below */
#endif

#elif defined(__TURBOC__)
#include <dos.h> /* inport[b](), outport[b]() */

#define inportw(P)      inport(P)
#define outportw(P,V)   outport(P,V)
/* [in|out]portl() defined below */

#else
#include <conio.h>
#endif

#define PCI_ADDR_REG            0xCF8
#define PCI_DATA_REG            0xCFC
/*****************************************************************************
These functions are _code_, but are stored in the _data_ segment.
Declare them 'far' and end them with 'retf' instead of 'ret'.

For 16-bit Watcom C, use 'cdecl' to force usage of normal, stack
calling convention instead of Watcom register calling convention.
*****************************************************************************/
#if defined(__TURBOC__) || (defined(__WATCOMC__)&&!defined(__386__))
static const unsigned char g_inportl[] =
{                           /* BITS 16 */
	0x55,                   /* push bp */
	0x8B, 0xEC,             /*  mov bp,sp */
	0x8B, 0x56, 0x06,       /*  mov dx,[bp + 6] */
	0x66, 0xED,             /*  in eax,dx */
	0x8B, 0xD0,             /*  mov dx,ax */
	0x66, 0xC1, 0xE8, 0x10, /*  shr eax,16 */
	0x92,                   /*  xchg dx,ax */
	0x5D,                   /* pop bp */
	0xCB                    /* retf */
};

static unsigned long far cdecl (*inportl)(unsigned port) =
        (unsigned long far cdecl (*)(unsigned))g_inportl;
/*****************************************************************************
*****************************************************************************/
static const unsigned char g_outportl[] =
{                           /* BITS 16 */
	0x55,                   /* push bp */
	0x8B, 0xEC,             /*  mov bp,sp */
	0x8B, 0x56, 0x06,       /*  mov dx,[bp + 6] */
	0x66, 0x8B, 0x46, 0x08, /*  mov eax,[bp + 8] */
	0x66, 0xEF,             /*  out dx,eax */
	0x5D,                   /* pop bp */
	0xCB                    /* retf */
};

static void far cdecl (*outportl)(unsigned port, unsigned long val) =
        (void far cdecl (*)(unsigned, unsigned long))g_outportl;
#endif
/*****************************************************************************
*****************************************************************************/
static int pci_detect(void)
{
	printf("PCI controller...");
	// poke 32-bit I/O register at 0xCF8 to see if
	// there's a PCI controller there
	outportl(PCI_ADDR_REG, 0x80000000L);
	if(inportl(PCI_ADDR_REG) != 0x80000000L)
	{
			printf("not found\n");
			return -1;
	}
	printf("found\n");
	return 0;
}
/*****************************************************************************
'bdf' = bus (b15-b8), device (b7-b3), and function (b2-b0)

Bitfields for PCI configuration address port:
Bit(s)  Description     (Table P0944)
 1-0    reserved (00)
 7-2    configuration register number (see #00878)
 10-8   function
 15-11  device number
 23-16  bus number
 30-24  reserved (0)
 31     enable configuration space mapping
*****************************************************************************/
typedef unsigned bdf_t;

static unsigned pci_read_byte(bdf_t bdf, unsigned reg)
{
/* Turbo C shift bug */
#if 0
	/* "enable configuration space mapping" */
	outportl(PCI_ADDR_REG, 0x80000000L
			| ((unsigned long)bdf << 8) | (reg & ~3));
#else
	unsigned long i = bdf;

	i <<= 8;
	i |= 0x80000000L; /* "enable configuration space mapping" */
	i |= (reg & ~3);
	outportl(PCI_ADDR_REG, i);
#endif
	return inportb(PCI_DATA_REG + (reg & 3));
}
/*****************************************************************************
note that b0 of 'reg' is ignored
*****************************************************************************/
static unsigned pci_read_word(bdf_t bdf, unsigned reg)
{
/*  outportl(PCI_ADDR_REG, 0x80000000L
            | ((unsigned long)bdf << 8) | (reg & ~3)); */
	unsigned long i = bdf;

	i <<= 8;
	i |= 0x80000000L;
	i |= (reg & ~3);
	outportl(PCI_ADDR_REG, i);
	return inportw(PCI_DATA_REG + (reg & 2));
}
/*****************************************************************************
note that b0 and b1 of 'reg' are ignored
*****************************************************************************/
static unsigned long pci_read_dword(bdf_t bdf, unsigned reg)
{
/*  outportl(PCI_ADDR_REG, 0x80000000L
            | ((unsigned long)bdf << 8) | (reg & ~3)); */
	unsigned long i = bdf;

	i <<= 8;
	i |= 0x80000000L;
	i |= (reg & ~3);
	outportl(PCI_ADDR_REG, i);
	return inportl(PCI_DATA_REG);
}
/*****************************************************************************
*****************************************************************************/
static void pci_write_byte(bdf_t bdf, unsigned reg, unsigned val)
{
/*      outportl(PCI_ADDR_REG, 0x80000000L
                | ((unsigned long)bdf << 8) | (reg & ~3)); */
        unsigned long i = bdf;

        i <<= 8;
        i |= 0x80000000L;
        i |= (reg & ~3);
        outportl(PCI_ADDR_REG, i);
// xxx - is this right?
        outportb(PCI_DATA_REG + (reg & 3), val);
}
/*****************************************************************************
note that b0 of 'reg' is ignored
*****************************************************************************/
static void pci_write_word(bdf_t bdf, unsigned reg, unsigned val)
{
/*      outportl(PCI_ADDR_REG, 0x80000000L
                | ((unsigned long)bdf << 8) | (reg & ~3)); */
        unsigned long i = bdf;

        i <<= 8;
        i |= 0x80000000L;
        i |= (reg & ~3);
        outportl(PCI_ADDR_REG, i);
        outportw(PCI_DATA_REG + (reg & 2), val);
}
/*****************************************************************************
note that b0 and b1 of 'reg' are ignored
*****************************************************************************/
static void pci_write_dword(bdf_t bdf, unsigned reg, unsigned long val)
{
/*      outportl(PCI_ADDR_REG, 0x80000000L
                | ((unsigned long)bdf << 8) | (reg & ~3)); */
        unsigned long i = bdf;

        i <<= 8;
        i |= 0x80000000L;
        i |= (reg & ~3);
        outportl(PCI_ADDR_REG, i);
        outportl(PCI_DATA_REG, val);
}
/*****************************************************************************
'bdf' = bus (b15-b8), device (b7-b3), and function (b2-b0)
*****************************************************************************/
#define FN(BDF)         (((BDF) >> 0) & 0x03)
#define DEV(BDF)        (((BDF) >> 3) & 0x1F)
#define BUS(BDF)        (((BDF) >> 8) & 0xFF)


static int pci_iterate(bdf_t *bdf_ptr)
{
	unsigned char hdr_type, mf = 0;
	bdf_t bdf = *bdf_ptr, next;

	// if first function of device, check if multi-function device
	// (otherwise fn==0 is the _only_ function of this device)
	if(FN(bdf) == 0)
	{
		// 0x0E=PCI_HEADER_TYPE (byte)
		hdr_type = pci_read_byte(bdf, 0x0E);
		// error in spec? I think they got this bit inverted...
		// "Bit 7 in this register is used to identify a multi-function device.
		// If the bit is 0, then the device is single function. If the bit is 1,
		// then the device has multiple functions.
		mf = ((hdr_type & 0x80) == 0);
	}
	// advance to next device if current device is NOT multi-function...
	if(mf)
		next = bdf + 0x08;
	// ...else advance to next function in current device
	// (and next device if current function = 7)
	else
			next = bdf + 1;
	// (...and advance to next bus if current device = 31)

	//if(BUS(next) != BUS(bdf) && BUS(next) > g_last_pci_bus)

	//if(next == 0) return 1; /* done */
	if(next > 999) return 1;  // GB 2014, make it stop

	*bdf_ptr = next;

	return 0;
}
/*****************************************************************************
"class" is a C++ reserved word
*****************************************************************************/
struct class_names_s
{
	const char *classname;
	const char **namelist;
	unsigned namecount;
};
static void display_device_class(unsigned _class, unsigned subclass)
{
	static const char *name1[] = {
		"SCSI", "IDE", "floppy", "IPI", "RAID"
	};
	static const char *name2[] = {
		"Ethernet", "Token Ring", "FDDI", "ATM"
	};
	static const char *name3[] = {
		"VGA", "SuperVGA", "XGA"
	};
	static const char *name4[] = {
		"video", "audio"
	};
	static const char *name5[] = {
		"RAM", "Flash"
	};
	static const char *name6[] = {
		"CPU", "ISA", "EISA", "MicroChannel", "PCI", "PCMCIA",
		"NuBus", "CardBus"
	};
	static const char *name7[] = {
		"PC serial", "PC parallel"
	};
	static const char *name8[] = {
		"8259 PIC", "8237 DMAC", "8254 PTC", "MC146818 RTC"
	};
	static const char *name9[] = {
		"keyboard", "digitizer/pen", "mouse"
	};
	static const char *name10[] = {
		"generic"
	};
	static const char *name11[] = {
		"386", "486", "Pentium", "Pentium Pro (P6)"
		/* 0x10=DEC Alpha; 0x40=coprocessor */
	};
	static const char *name12[] = {
		"Firewire (IEEE 1394)", "ACCESS.bus", "SSA", "USB",
		"Fiber Channel"
	};

	static struct class_names_s cnames[] = {
		{ "", NULL, 0 },
		{ "disk controller", name1, sizeof(name1) / sizeof(name1[0]) },
		{ "network controller", name2, sizeof(name2) / sizeof(name2[0]) },
		{ "display controller", name3, sizeof(name3) / sizeof(name3[0]) },
		{ "multimedia controller", name4, sizeof(name4) / sizeof(name4[0]) },
		{ "memory", name5, sizeof(name5) / sizeof(name5[0]) },
		{ "bridge", name6, sizeof(name6) / sizeof(name6[0]) },
		{ "communications device", name7, sizeof(name7) / sizeof(name7[0]) },
		{ "system device", name8, sizeof(name8) / sizeof(name8[0]) },
		{ "HID", name9, sizeof(name9) / sizeof(name9[0]) },
		{ "dock", name10, sizeof(name10) / sizeof(name10[0]) },
		{ "CPU", name11, sizeof(name11) / sizeof(name11[0]) },
		{ "serial bus controller", name12, sizeof(name12) / sizeof(name12[0]) },
	};

	if(_class >= 1 || _class < sizeof(cnames)/sizeof(cnames[0]))
	{
		printf("%s", cnames[_class].classname);
		if(subclass >= cnames[_class].namecount)
		{
			printf(":unknown (subclass=%u)", subclass);
			return;
		}
		printf(":%s", cnames[_class].namelist[subclass]);
	}
	else
	{
			printf("unknown (class=%u)", _class);
	}
}

//------------------------------------------------------------------------------

int header;
static void
print_scan_entry(unsigned bdf, unsigned vendor, unsigned device,
                 unsigned long devclass)
{
	unsigned _class, subclass, api;

	if(header == 0)
	{
		printf("Bus Device Function VendorID DeviceID Class:Subclass:API\n"
		       "--- ------ -------- -------- -------- ------------------\n");
	}
	header = 1;

	printf("%3u %6u %8u %8X %-8X ",
	       BUS(bdf), DEV(bdf), FN(bdf), vendor, device);

	/* 09=PCI_PROGRAMMING_INTERFACE, 0A=PCI_SUBCLASS, 0B=PCI_CLASS */
	_class   = (unsigned)((devclass >> 24) & 0xFF);
	subclass = (unsigned)((devclass >> 16) & 0xFF);
	api      = (unsigned)((devclass >>  8) & 0xFF);

	display_device_class(_class, subclass);
	printf(":%02X %08X\n", api, bdf);
}

// Finds the BDF for a given PCI device.  Returns non-zero if found and the bdf
// is written to the outbdf pointer.
static int
pci_lookup(unsigned target_vendor, unsigned target_device, unsigned *outbdf)
{
	unsigned long i;
	unsigned bdf;

	/* check for PCI */
	/* display numeric ID of all PCI devices detected */
	bdf = 0;

	*outbdf = 0;

    do
    {
		unsigned vendor, device;

		/* 00=PCI_VENDOR_ID (word), 02=PCI_DEVICE_ID (word) */
		i = pci_read_dword(bdf, 0x00);
		vendor = (unsigned)(i & 0xFFFF);
		device = (unsigned)(i >> 16);

		if (vendor == target_vendor && device == target_device)
		{
			i = pci_read_dword(bdf, 0x08);
			print_scan_entry(bdf, vendor, device, i);

			*outbdf = bdf;
			return 1; // done
		}
	}
	while(!pci_iterate(&bdf));
	return 0;
}

// End of PCI.C code -----------------------------------------------------------

#define ALI_PCI_VENDOR 0x10B9u
#define ALI_M15X3_BRIDGE 0x1533u
#define ALI_PMU_CONFIG 0x7101u

#define ALI15X3_REG_PMU 0x5F
#define ALI15X3_PMU_EN 0x4

#define ALIPMU_REG_REGCTRL 0x5B
#define ALIMPU_THROT_BREAK_HIGH 0x40
#define ALIPMU_THROT_PERIOD_8us 0x20
#define ALIPMU_P_CNT_LOCK 0x10
#define ALIPMU_THROT_BREAK_LOW 0x8
#define ALIPMU_SMBIO_LOCK 0x4
#define ALIPMU_ACPIIO_LOCK 0x2
#define ALIPMU_STPCLK_SELF_REFRESH 0x1
#define ALIPMU_LOCK (ALIPMU_SMBIO_LOCK|ALIPMU_ACPIIO_LOCK)

#define ALIPMUIO_REG_THROTTLE 0x10 //0x10-0x13
#define ALIPMUIO_CLK_EN 0x200l
#define ALIPMUIO_THRO_EN 0x10l
#define ALIPMUIO_THRO_DTY_MASK 0xEl

// Find an unused port address range (for PMU)
static unsigned
find_base_addr()
{
	unsigned addr, i, good;
	for(addr = 0x9000; addr < 0xF000; addr += 0x100)
	{
		good = 1;
		for(i = 0;i < 256;i += 2)
		{
			unsigned result;
			result = inportw(addr+i);
			if(result != 0xFFFFu)
			{
				good = 0;
				break;
			}
		}

		if(good)
			return addr;
	}
	return 0;
}

// Sets the PMU IO base address
static void
program_pmu_base_addr(bdf_t pmubdf, unsigned addr)
{
	unsigned char val;

	// Disable IO
	val = pci_read_byte(pmubdf, 0x4);
	val &= ~1;
	pci_write_byte(pmubdf, 0x4, val);

	// Porgram address
	addr = (addr&0xFFC0u)|1;
	pci_write_word(pmubdf, 0x10, addr);

	// Enable IO
	val = pci_read_byte(pmubdf, 0x4);
	val |= 1;
	pci_write_byte(pmubdf, 0x4, val);
}

// GB 2014
// Checks whether PMU and SMB are enabled and turns them on in case they are
// not. It's done by clearing Bit 2 in M1533 config space 5Fh.
static int
unlock_pmu(bdf_t sbbdf)
{
	unsigned char val;

	val = pci_read_byte(sbbdf, ALI15X3_REG_PMU);
	if (val & ALI15X3_PMU_EN) 
	{
		val = val & (~ALI15X3_PMU_EN);
		pci_write_byte(sbbdf, ALI15X3_REG_PMU, val);
		val = pci_read_byte(sbbdf, ALI15X3_REG_PMU);
	}

	return (val & ALI15X3_PMU_EN) == ALI15X3_PMU_EN;
}

static void
unlock_pmu_registers(bdf_t pmubdf)
{
	unsigned char val;

	val = pci_read_byte(pmubdf, ALIPMU_REG_REGCTRL);
	if(val & ALIPMU_LOCK) // bit 1 and 2 are set (value 6)
	{
		printf("Unlocking ALi m7101 address registers: %02X\n", val);
		val &= ~ALIPMU_LOCK;
		pci_write_byte(pmubdf, ALIPMU_REG_REGCTRL, val);
	}
	else
		printf("ALi m7101 registers already unlocked: %02X\n", val);
}

static void
pmuio_set_throttle(bdf_t pmubdf, unsigned amount)
{
	unsigned val;
	unsigned baseaddr;

	baseaddr = find_base_addr();
	program_pmu_base_addr(pmubdf, baseaddr);

	// Clear throttling
	val = inportw(baseaddr+ALIPMUIO_REG_THROTTLE);
	val &= ~(ALIPMUIO_CLK_EN|ALIPMUIO_THRO_EN|ALIPMUIO_THRO_DTY_MASK);
	outportw(baseaddr+ALIPMUIO_REG_THROTTLE, val);

	if(amount > 0)
	{
		// Data sheet says bit 9 should be enabled first before anything else
		// so lets do that even if it doesn't seem to be necessary.
		val |= ALIPMUIO_CLK_EN;
		outportw(baseaddr+ALIPMUIO_REG_THROTTLE, val);

		// Turn on throttling
		val |= ((8-amount)<<1)|ALIPMUIO_THRO_EN;
		outportw(baseaddr+ALIPMUIO_REG_THROTTLE, val);
		printf("Throttle enabled: %04X\n", val);
	}
	else
	{
		printf("Throttle disabled: %04X\n", val);
	}
}

static int
do_throttle(bdf_t sbbdf, bdf_t pmubdf, unsigned amount)
{
	// Recheck in case it was relocked
	unlock_pmu(sbbdf);

	unlock_pmu_registers(pmubdf);

	pmuio_set_throttle(pmubdf, amount);
	return 0;
}

static int
prompt_amount()
{
	unsigned amount;

	printf("\n"
	       "Speed options:\n"
	       "0. top speed              (No throttle)\n"
	       "1. not really slow at all (12.5% throttled)\n"
	       "2. sorta kinda slow       (25% throttled)\n"
	       "3. gettin' there          (37.5% throttled)\n"
	       "4. half slow              (50% throttled)\n"
	       "5. pretty slow            (62.5% throttled)\n"
	       "6. considerably slow      (75% throttled)\n"
	       "7. whoa! Is it 1993?      (87.5% throttled)\n"
	       "ESC. get me outta here. No changes.\n"
	       "\n");

	amount = getch();
	if(amount < '0' || amount > '7')
		return -1;

	return amount - '0';
}

int main(int argc, char* argv[])
{
	bdf_t sbbdf, pmubdf;
	int returncode=0;
	int amount=-1;

	printf("ALi Aladdin V ACPI Throttle - G. Broers, Braden Obrzut, thanks to Chris Giese.\n");
	if (pci_detect())
		return 1;

	returncode=pci_lookup(ALI_PCI_VENDOR, ALI_M15X3_BRIDGE, &sbbdf);
	if (returncode)
	{
		unlock_pmu(sbbdf);
		returncode=pci_lookup(ALI_PCI_VENDOR, ALI_PMU_CONFIG, &pmubdf);
	}
	if (returncode==0)
	{
		printf("No m7101 PMU Detected.");
		return 1;
	}

	// Get throttle amount
	if(argc > 1)
		amount = argv[1][0]-'0';
	if(amount < 0 || amount > 7)
		amount = prompt_amount();
	// Aborted?
	if(amount < 0)
		return 0;

	returncode = do_throttle(sbbdf, pmubdf, amount);
	return returncode; // returns 1 if found 
}
