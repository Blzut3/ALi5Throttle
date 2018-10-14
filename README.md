ALi Aladdin V ACPI Throttle Utility

Version 0.10

USAGE 
-----
ALi5THT is a computer slowdown utility. It uses the Motherboard southbridge ACPI
functions to introduce wait states, and being hardware based does not run in The
background. This package allows one to use ACPI throttling on ALi Aladdin V
motherboards. 

Run ALi5THT and select the desired amount of slowdown or pass it as a parameter
without a prompt.

BACKGROUND
----------
The original Throttle does not work with tested ALi Aladdin V based motherboards, 
such as the Asus P5A, Gigabyte GA-5AX, Biostar M5ALA, Chaintech 5RSA, BCM VP1543.
There are four parts that needed to be tweaked to make things work:
- ALi m7101 PMU* needs to be enabled, if the BIOS has not done this already,
  by setting a bit in the ALi m1533 Bridge Device.
- Unset ALi m7101 PMU registers write protection bits.
- At every run of throttle, BIT9 of the ACPI I/O space is now set.
  This is just outside of the range of the normal Throttle routines.
- The ALi 5 Throttle I/O space address register was changed to 0x10h.

This was originally modification of the Throttle utility with an additional
program to unlock the PMU.  However, at least some boards have been observed to
re-lock the PMU in the background.  Thus under high degrees of throttling it
would sometimes take too long for throttle to load to be able to change or
remove throttling.  The task of setting the throttle register has thus been
integrated into the single utility.

Compared to throttle this program does not disable the CPU L1 cache.  The focus
of this utility is only the motherboard.  If this behavior is desired the
[SetMul](https://www.vogons.org/viewtopic.php?f=24&t=38613) or similar utility
may be used to tweak the CPU.

*PMU= Power Management Unit

FILE DESCRIPTIONS
-----------------
- ALI5THT.EXE - Throttle utility specific for ALi Aladdin V Southbridge.
- ALI5THT.TXT - This text file.

CREDITS
-------
ALi 5 throttle by G. Broers, March 2014.

Based on PCI.C by Chris Giese.

Inspired by original Throttle by Jeff Leyda,  Dec 15, 2007.

Throttling ported to C and tool unified by Braden "Blzut3" Obrzut, Oct 13, 2018


WARNING
-------
I noticed a nasty lockup when using 'SetMul' or other K6+ clock utilities to
toggle the multiplier while throttle was enabled. Using SetMul prior to, and
after closing Throttle is fine.

The Biostar M5ALA seems to be extra quirky in that throttling may hard lock
the system when used under DOS.  This may lock hard enough to require a power
supply power cycle!  The program works fine under Windows though.

The M5ALA is also known to have a lock up issue relating to the
[external cache](http://maniacsvault.net/entry85), so it's possible that this
board just poorly initializes at the BIOS.  As of this writing it is suspected
that some additional registers just need to be set properly.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
