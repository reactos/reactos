// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

// Mouse commands
#define MOUSE_SET_RESOLUTION            0xE8  // Set resolution
#define MOUSE_SET_SCALE11               0xE6  // Set 1:1 scaling
#define MOUSE_SET_SCALE21               0xE7  // Set 2:1 scaling
#define MOUSE_GET_SCALE                 0xE9  // Get scaling factor
#define MOUSE_SET_STREAM                0xEA  // Set stream mode
#define MOUSE_READ_DEVICETYPE			0xF2  // Read Device Type
#define MOUSE_SET_SAMPLE_RATE           0xF3  /* Set sample rate (number of times
                                               * the controller will poll the port
                                               * per second */
#define MOUSE_ENABLE_DEVICE             0xF4  // Enable mouse device
#define MOUSE_DISABLE_DEVICE            0xF5  // Disable mouse device
#define MOUSE_RESET                     0xFF  // Reset aux device
#define MOUSE_ACK			0xFA  // Command byte ACK

#define MOUSE_INTERRUPTS_OFF            (CONTROLLER_MODE_KCC | \
                                         CONTROLLER_MODE_DISABLE_MOUSE | \
                                         CONTROLLER_MODE_SYS | \
                                         CONTROLLER_MODE_KEYBOARD_INTERRUPT)

#define MOUSE_INTERRUPTS_ON             (CONTROLLER_MODE_KCC | \
                                         CONTROLLER_MODE_SYS | \
                                         CONTROLLER_MODE_MOUSE_INTERRUPT | \
                                         CONTROLLER_MODE_KEYBOARD_INTERRUPT)

// Used with mouse buttons
#define GPM_B_LEFT      1
#define GPM_B_RIGHT     2
#define GPM_B_MIDDLE    4
#define GPM_B_FOURTH    0x10
#define GPM_B_FIFTH     0x20

// Some aux operations take long time
#define MAX_RETRIES          60

// Hardware defines
#define MOUSE_IRQ            12
#define MOUSE_WRAP_MASK      0x1F

#define MOUSE_ISINTELLIMOUSE    0x03
#define MOUSE_ISINTELLIMOUSE5BUTTONS    0x04

// -----------------------------------------------------------------------------

#define WHEEL_DELTA 120

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

#define PSMOUSE_PS2	1
#define PSMOUSE_PS2PP	2
#define PSMOUSE_PS2TPP	3
#define PSMOUSE_GENPS	4
#define PSMOUSE_IMPS	5
#define PSMOUSE_IMEX	6
#define PSMOUSE_SYNAPTICS 7

#define input_regs(a,b)		do { (a)->regs = (b); } while (0)

static PIRP  CurrentIrp;
static ULONG MouseDataRead;
static ULONG MouseDataRequired;
static BOOLEAN AlreadyOpened = FALSE;
static KDPC MouseDpc;

static VOID MouseDpcRoutine(PKDPC Dpc,
			  PVOID DeferredContext,
			  PVOID SystemArgument1,
			  PVOID SystemArgument2);
