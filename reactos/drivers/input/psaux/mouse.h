// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

#define WHEEL_DELTA (120)

#define PSMOUSE_CMD_SETSCALE11	0x00e6
#define PSMOUSE_CMD_SETRES	0x10e8
#define PSMOUSE_CMD_GETINFO	0x03e9
#define PSMOUSE_CMD_SETSTREAM	0x00ea
#define PSMOUSE_CMD_POLL	0x03eb	
#define PSMOUSE_CMD_GETID	0x02f2
#define PSMOUSE_CMD_SETRATE	0x10f3
#define PSMOUSE_CMD_ENABLE	0x00f4
#define PSMOUSE_CMD_RESET_DIS	0x00f6
#define PSMOUSE_CMD_RESET_BAT	0x02ff

#define PSMOUSE_RET_BAT		0xaa
#define PSMOUSE_RET_ACK		0xfa
#define PSMOUSE_RET_NAK		0xfe

#define MOUSE_INTERRUPTS_OFF            (CONTROLLER_MODE_KCC | \
                                         CONTROLLER_MODE_DISABLE_MOUSE | \
                                         CONTROLLER_MODE_SYS | \
                                         CONTROLLER_MODE_KEYBOARD_INTERRUPT)

#define MOUSE_INTERRUPTS_ON             (CONTROLLER_MODE_KCC | \
                                         CONTROLLER_MODE_SYS | \
                                         CONTROLLER_MODE_MOUSE_INTERRUPT | \
                                         CONTROLLER_MODE_KEYBOARD_INTERRUPT)

// Used with mouse buttons
#define GPM_B_LEFT      0x01
#define GPM_B_RIGHT     0x02
#define GPM_B_MIDDLE    0x04
#define GPM_B_FOURTH    0x10
#define GPM_B_FIFTH     0x20

// Mouse types
#define PSMOUSE_PS2	1
#define PSMOUSE_PS2PP	2
#define PSMOUSE_PS2TPP	3
#define PSMOUSE_GENPS	4
#define PSMOUSE_IMPS	5
#define PSMOUSE_IMEX	6
#define PSMOUSE_SYNAPTICS 7

// Some aux operations take long time
#define MAX_RETRIES          60

// Hardware defines
#define MOUSE_IRQ            12
#define MOUSE_WRAP_MASK      0x1F

#define MOUSE_ISINTELLIMOUSE    0x03
#define MOUSE_ISINTELLIMOUSE5BUTTONS    0x04

static PIRP  CurrentIrp;
static ULONG MouseDataRead;
static ULONG MouseDataRequired;
static BOOLEAN AlreadyOpened = FALSE;
static KDPC MouseDpc;

static VOID MouseDpcRoutine(PKDPC Dpc,
			  PVOID DeferredContext,
			  PVOID SystemArgument1,
			  PVOID SystemArgument2);
