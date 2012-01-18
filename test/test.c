#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "test_suite.h"

#ifndef TEST_SUITE
#error "Please use -DTEST_SUITE='"sth"' to define a test suite ..."
#endif

#include TEST_SUITE

// main function starting the test suite
int main( int argc, char **argv)
{
        int starting_test = 0;
        
        // create array for saving failed tests
        int test_count = sizeof(tests)/sizeof(test_case);

        switch( argc )
        {
                case 1:
                        break;

                case 3: 
                        test_count = min( test_count, atoi(argv[2])+1);
                case 2:
                        starting_test = atoi(argv[1]);
                        break;

                default:
                        fprintf(stderr, "usage %s [FIRST_TEST] [LAST_TEST]\n", argv[0] );
                        return -1;
                        break;
        }

        int *failed_tests = (int*)malloc(sizeof(int) * test_count);
        int last_failed_test = 0;

        if( starting_test < test_count && starting_test >= 0 && test_count >= 0)  
        {
                printf("\nTesting: ");
        }
        else
        {
                printf("No tests selected, exiting.\n");
                return 1;
        }

        // loop through all tests
        for(int test_id = starting_test; test_id < test_count; ++test_id)
        {
                setup_test();
                test_case test = tests[test_id];
                
                int result = test.function();
                if( !result )
                {
                        printf( "\e[31mF\e[m" );
                        failed_tests[last_failed_test++] = test_id;
                }
                else
                {
                        printf( "\e[32m.\e[m" );
                }
                cleanup_test();

                fflush(stdout);
        }

        // print all failed tests
        if( last_failed_test != 0)
        {
                // display all failed tests
                printf("\n\nFollowing tests failed:\n");
                for( int failed_test = 0; failed_test < last_failed_test; ++failed_test )
                {
                        int id = failed_tests[failed_test];
                        printf("\t%3d: %s\n", id, tests[id].description );
                }
        }
        printf("\n");

        // cleanup
        free(failed_tests);
        return last_failed_test;
}
	
