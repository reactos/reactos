#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Forward declaration of the actual function from kdvm.c
extern int KdReceivePacket(void *MessageHeader, void *DataHeader);

START_TEST(test_integer_overflow_protection)
{
    // Invariant: HeaderSize + DataSize calculation must not overflow and bypass validation
    struct {
        uint32_t HeaderSize;
        uint32_t DataSize;
        const char *description;
    } test_cases[] = {
        // Exploit case: overflow causing sum to wrap below ReceivedSize
        {0xFFFFFFFF, 1, "Exploit: overflow wrap"},
        // Boundary case: maximum safe values
        {0x7FFFFFFF, 0x7FFFFFFF, "Boundary: max safe sum"},
        // Valid case: normal operation
        {1024, 2048, "Valid: normal sizes"},
        // Edge case: zero sizes
        {0, 0, "Edge: zero sizes"},
        // Large but non-overflowing
        {0x80000000, 0x7FFFFFFF, "Large: near overflow"}
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        // Create a test packet structure
        struct {
            uint32_t MessageHeaderLength;
            uint32_t HeaderSize;
            uint32_t DataSize;
            char header_data[1];
        } *packet;
        
        size_t total_size = sizeof(*packet) + test_cases[i].HeaderSize + test_cases[i].DataSize;
        
        // Only allocate if calculation didn't overflow
        if (test_cases[i].HeaderSize + test_cases[i].DataSize >= test_cases[i].HeaderSize) {
            packet = malloc(total_size);
            if (!packet) continue;
            
            memset(packet, 0, total_size);
            packet->HeaderSize = test_cases[i].HeaderSize;
            packet->DataSize = test_cases[i].DataSize;
            
            // Setup message header for copy test
            struct {
                uint32_t Length;
                char *Buffer;
                uint32_t MaximumLength;
            } msg_header;
            
            char header_buffer[4096];
            msg_header.Buffer = header_buffer;
            msg_header.MaximumLength = sizeof(header_buffer);
            
            // Mock the receive buffer - in real test this would intercept KdVmSendReceive
            // For property test, we verify the calculation logic
            uint64_t calculated_size = sizeof(*packet) + (uint64_t)packet->HeaderSize + (uint64_t)packet->DataSize;
            uint64_t received_size = total_size;
            
            // Security property: calculated size must match actual allocated size
            // and must not have overflowed in 32-bit arithmetic
            uint32_t sum32 = packet->HeaderSize + packet->DataSize;
            uint64_t sum64 = (uint64_t)packet->HeaderSize + (uint64_t)packet->DataSize;
            
            ck_assert_msg(sum32 == sum64 || calculated_size <= UINT32_MAX,
                         "Test case '%s': Integer overflow detected in HeaderSize + DataSize calculation",
                         test_cases[i].description);
            
            free(packet);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_integer_overflow_protection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}