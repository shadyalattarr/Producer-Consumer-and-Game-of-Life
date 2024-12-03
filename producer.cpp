#include <iostream>
#include <string>
#include <cstdlib> // For atof()
#include <time.h>
#include <ctime>
#include <unistd.h>
#include <random>

using namespace::std;

int main(int argc, char *argv[]) {
    if (argc != 6) { // Check for the correct number of arguments
        cerr << "Usage: ./producer <commodity> <value1> <mean> <interval> <buffer_size>\n";
        return 1;
    }

    string commodity = argv[1];       // First argument (e.g., NATURALGAS)
    if(commodity.length() > 10){
        cerr << "Commodity name can not be more than 10 characters\n";
        return 2;
    }

    double price_mean = atof(argv[2]);         // Convert second argument to double
    double price_sd = atof(argv[3]);           // Convert third argument to double
    int wait_interval = std::atoi(argv[4]);     // Convert fourth argument to int
    int buffer_size = std::atoi(argv[5]);  // Convert fifth argument to int

    // Output the parsed values
    //cout << "argv[0] :" << argv[0] << "\n";
    cout << "Commodity: " << commodity << "\n";
    cout << "Price mean: $ " << price_mean << "\n";
    cout << "Price standard deviation: $ " << price_sd << "\n";
    cout << "Interval: " << wait_interval << " ms\n";
    cout << "Buffer size: " << buffer_size << "\n";
    
    default_random_engine generator;
    normal_distribution<double> distribution(price_mean,price_sd);
    // first let's try and produce into terminal instead into a prod log file
    while(true)
    {
        struct timespec tp;
        if (clock_gettime(CLOCK_REALTIME, &tp) == -1) {
            cerr << "Problem with retrieving time.\n";
            return 3;
        }

        time_t timestamp = time(NULL);
        struct tm datetime = *localtime(&timestamp);
        char output[50];
        long milliseconds = tp.tv_nsec / 1000000; // turn ns to ms

        // Format the date and time string
        strftime(output, 50, "%m/%d/%Y %H:%M:%S", &datetime);

        // Print using printf
        printf("[%s.%03ld] ", output, milliseconds); // 0 padding
        printf("Price generated: %f\n",distribution(generator));
        usleep(wait_interval * 1000); // usleep uses micro and we got milli
    }
    
    return 0;
}
