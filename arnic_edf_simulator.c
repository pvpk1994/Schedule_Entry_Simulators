/********** EDF ARINC SCHEDULE ENTRY SIMULATOR ********
 * Author: Pavan Kumar Paluri
 * Developed @ RT-LAB @ UNIVERSITY OF HOUSTON
 * Month/Year: Apr/2020
 * ****************************************************/

/************ ASSUMPTIONS ************
 Update: This Schedule entry simulator assumes partitions to follow implicit-deadline nature
          i.e., period_of_partition_i == deadline_of_partition_i
 Update: All Domains are assumed to have arrival times to be 0 for simplicity.
 Update: There will not be more than 1000 processes (For simplicity)
 **************************************/

// TODO: In Xen Compliant ARINC EDF simulator, change partition_id to Domain's UUID

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int64_t s_time_t;
#define MILLISECS(_ms)  (((s_time_t)(_ms)) * 1000000UL )
#define DOMN_RUNTIME    MILLISECS(30) // Default runtime of 30ms timeslice

struct partition {
    int partition_id;
    int wcet;
    int period; // period == deadline
};

// Conditionals
#define PRINT_ENABLE
#define PRINT_ENABLE_NI

// Function Declarations
void edf_schedule(struct partition*, int, int);

/******** HELPER FUNCTIONS ***********/
int raise_error(double result)
{
    printf("Schedulability is: %f >= 1.00\n", result);
    printf("Exiting the program ....\n");
    return EXIT_FAILURE;
}

int lcm(int num1, int num2)
{
    int minMultiple;
    minMultiple = (num1 > num2) ? num1 : num2;
    while (1)
    {
        if(minMultiple%num1 == 0 && minMultiple%num2 == 0)
            break;
        ++minMultiple;
    }
    return minMultiple;
}

int cal_hp(struct partition *sched_entry, int total_entries)
{
    int final_val = 1;
    for(int i=0; i< total_entries; i++)
        final_val = lcm(final_val, sched_entry[i].period);
    return final_val;
}
/*********** END OF HELPER FNS ******************/

int main()
{
    int number_of_partitions, hyperperiod_res;
    double sched_result = 0.0;
    printf("Enter the number of partitions: ");
    scanf("%d", &number_of_partitions);
    struct partition *partitions;
    partitions = (struct partition*) malloc(number_of_partitions * sizeof(struct partition));
    for(int i =0; i <number_of_partitions; i++)
    {
        // Partition_ID loader
        partitions[i].partition_id = i+1;
        printf("Enter the wcet of Partition %d: ", i);
        scanf("%d", &partitions[i].wcet);

        printf("Enter the period of Partition %d: ", i);
        scanf("%d", &partitions[i].period);
    }

    // Print
#ifdef PRINT_ENABLE
for(int i=0; i < number_of_partitions; i++)
    {
        printf("Partition %d:  wcet: %d  period: %d\n", partitions[i].partition_id, partitions[i].wcet, partitions[i].period);
    }
#endif

    // necessary and sufficient schedulability test
    // if Sum of (c_i/p_i) <= 1: then schedulable, else No
    for(int i=0; i < number_of_partitions; i++)
    {
        sched_result += (double)partitions[i].wcet/ partitions[i].period;
    }

    sched_result <= 1 ? printf("Schedulable and Utilization is: %f \n", sched_result) : raise_error(sched_result);

    // Calculate Hyper-period for the schedule
    hyperperiod_res = cal_hp(partitions, number_of_partitions);

    #ifdef PRINT_ENABLE
    printf("Hyperperiod of the schedule is: %d\n", hyperperiod_res);
#endif

    // Compute EDF schedule
    edf_schedule(partitions, number_of_partitions, hyperperiod_res);
    return 0;
}

void edf_schedule(struct partition* domain, int num_of_domains, int hp)
{
    int earliest_ddl;
    int next_ddl[1000];
    int rem_wcet[1000];
    int period[1000];
    int sched_table[hp];
    int earliest_ddl_index;
    int process_period[1000];
    for(int i=0; i<num_of_domains; i++)
    {
        next_ddl[i] = domain[i].period;
        rem_wcet[i] = domain[i].wcet;
        period[i] = next_ddl[i];
        // Period Container
        process_period[i] = 0;
    }
#ifdef PRINT_ENABLE
    printf("\n*******************************\n");
    printf("P-1: Represents IDLE Domain/VCPU\n");
    printf("********************************\n");
#endif
    // Iterate through each time-slice..
    for(int i=0; i<hp; i++)
    {
#ifdef PRINT_ENABLE
        printf("TS: [%d to %d] - ", i, i+1);
#endif
        // Set init val of earliest_ddl to be the hp
        earliest_ddl = hp;
        earliest_ddl_index = -1;
        // Which process to choose ??
        for(int j=0; j<num_of_domains; j++)
        {
        if(rem_wcet[j] > 0)
            {
                if(earliest_ddl >= next_ddl[j])
                {
#undef PRINT_ENABLE_NI
#ifdef PRINT_ENABLE_NI
                    printf("earliest_ddl: %d\n", earliest_ddl);
#endif
                    // Set earliest ddl to be this process's deadline.
                    earliest_ddl = next_ddl[j];
                    earliest_ddl_index = j;
                }
            }
        }
#ifdef PRINT_ENABLE
        printf(" P%d \n", earliest_ddl_index);
#endif
        // reduce the wcet of that partition by 1 time unit now..
        rem_wcet[earliest_ddl_index]--;

        for(int j=0; j<num_of_domains; j++)
        {
            if(process_period[j] == period[j]-1)
            {
                // Resetting phase...
                next_ddl[j] = domain[j].period;
                rem_wcet[j] = domain[j].wcet;
                period[j] = next_ddl[j];
                process_period[j] = 0;
            }
            else{
            if(next_ddl[j] > 0)
                next_ddl[j]--;
            else {
                if (rem_wcet[j] > 0)
                    printf("\n the Process %d has exceeded the deadline \n", j);
            }
            process_period[j]++;
            }
        }
    }
}

