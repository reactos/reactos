// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

// Mouse commands
#define MOUSE_SET_RESOLUTION            0xE8  // Set resolution
#define MOUSE_SET_SCALE11               0xE6  // Set 1:1 scaling
#define MOUSE_SET_SCALE21               0xE7  // Set 2:1 scaling
#define MOUSE_GET_SCALE                 0xE9  // Get scaling factor
#define MOUSE_SET_STREAM                0xEA  // Set stream mode
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

// Some aux operations take long time
#define MAX_RETRIES          60

// Hardware defines
#define MOUSE_IRQ            12
#define MOUSE_WRAP_MASK      0x1F

static PIRP  CurrentIrp;
static ULONG MouseDataRead;
static ULONG MouseDataRequired;
static BOOLEAN AlreadyOpened = FALSE;
static KDPC MouseDpc;

static VOID MouseDpcRoutine(PKDPC Dpc,
			  PVOID DeferredContext,
			  PVOID SystemArgument1,
			  PVOID SystemArgument2);

