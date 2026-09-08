#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//Using a 100 individuals population
#define POPULATION_SIZE 100
#define FX_LOZER_BOUND -20
#define FX_UPPER_BOUND 20
#define PRECISION 3

//Making a Individual like a struct
typedef struct ind_t {
    int* chromosome;
    double x; // Genes
    double fitness; // aptitude
    int parents[2]; // Parents of the individual
    int mutation_place; // Place where the mutation happened
    int crossover_place; // Place where the crossover happened

} Individual;

// Global variables for the populations
Individual* parents;
Individual* offspring;
double* roulette;

// Util variables
unsigned chromosome_length;
double crossover_probability;

// Function to serve the memory for the individuals
void allocateMemory() {
    unsigned required_bytes = POPULATION_SIZE * sizeof(Individual);
    parents = (Individual*)malloc(required_bytes);
    offspring = (Individual*)malloc(required_bytes);
    roulette = (double*)malloc(POPULATION_SIZE * sizeof(double));
    // Chromosome allocation for each individual
    chromosome_length = ceil(log2((FX_UPPER_BOUND - FX_LOZER_BOUND) * pow(10, PRECISION))); // Ceil function to the next nearest integer
    required_bytes = chromosome_length * sizeof(int);

    for (int i = 0; i < POPULATION_SIZE; i++) {
        parents[i].chromosome = (int*)malloc(required_bytes); // Chromosome allocation for each individual parent
        offspring[i].chromosome = (int*)malloc(required_bytes); // Chromosome allocation for each individual offspring
    }
}

//random number generator function from an interval [a, b]
double randomDouble(double a, double b) {
    return (b-a) * ((double)rand() / RAND_MAX) + a;
}

// Flip a coin function to get a random value of 0 or 1 based on a given probability
int flip(double probability) {
    return (randomDouble(0, 1) < probability) ? 0 : 1;
}

// creating the populations
void createFirstGeneration(){
     for(int i =0; i < POPULATION_SIZE; i++){
        parents[i].x = RAND_MAX; // Randomly assigning a value to x
        parents[i].fitness = 0; // Randomly assigning a value to fitness, the lowest value is 0 and the highest is 1
        parents[i].parents[0] = parents[i].parents[1] = -1; // No parents for the first generation in a double array of size 2, and equally initialized to -1
        parents[i].mutation_place = parents[i].crossover_place = -1;

        for(int j=0; j < chromosome_length; j++){
            parents[i].chromosome[j] = flip(0.5); // Randomly assigning 0 or 1 to each gene in the chromosome
        }
    }
}

//convert binary to decimal function
double binary2real(int* chromosome) {
    double aux = 0.0;
    for(int i=chromosome_length-1; i >= 0; i--){
        aux += chromosome[i] * pow(2, chromosome_length - 1 - i);
    }
    return FX_LOZER_BOUND + (( aux * (FX_UPPER_BOUND - FX_LOZER_BOUND) ) / (pow(2, chromosome_length) - 1));
}

// Evaluate the target function for an individual
void evaluateTargetFunction(Individual* individual) {
    individual -> x = binary2real(individual -> chromosome);
    individual -> fitness = 1/(pow(individual -> x, 2)+0.001); // Example target function: f(x) = x^2, and minimizing
    // we uses 0.001 to avoid division by zero when x is 0
}

// Evaluate the target function for the entire population
void evaluatePopulation(Individual* population) {
    for(int i = 0; i < POPULATION_SIZE; i++){
        evaluateTargetFunction(&population[i]);
    }
}

// Update the roulette wheel selection probabilities based on the fitness of the individuals
void updateRoulette(Individual* population) {
    double total_fitness = 0.0;
    // Calculate the total fitness of the population
    for (int i = 0; i < POPULATION_SIZE; i++) {
        total_fitness += population[i].fitness;
    }
    // Calculate the cumulative probabilities for each individual
    double cumulative_probability = 0.0;
    for (int i = 0; i < POPULATION_SIZE; i++) {
        cumulative_probability += population[i].fitness / total_fitness;
        roulette[i] = cumulative_probability;
    }
}

//Making the selection of the parents based on the roulette wheel selection method, and returning the index of the selected individual
unsigned rouletteWheelSelection() {
    double r = randomDouble(0, 1);
    double sum = 0.0;
    // the worst probability to have could be 1 
    for (int i = 0; sum < r; i++) {
        sum += roulette[i];
        if (sum >= r) {
            return i; // Return the index of the selected individual
        }
    }
    //return POPULATION_SIZE - 1; // Return the last individual if not found
}

void crossover(Individual* father, Individual* mother, Individual* child1, Individual* child2) {
    int i = 0;
    if(flip(crossover_probability)) {
        // Randomly select a crossover point
        unsigned p = (unsigned)radomDouble(1, chromosome_length - 2); // Randomly select a crossover point
        // Copy genes from parents to children based on the crossover point
        for(i = 0; i < p; i++) {
            child1->chromosome[i] = father->chromosome[i]; // Copy genes from father to child1 up to the crossover point
            child2->chromosome[p+i] = mother->chromosome[i]; // Copy genes from mother to child2 up to the crossover point
        }
        // Copy the remaining genes from the other parent to the children after the crossover point
        for(i = p+1; i < chromosome_length; i++) {
            child1->chromosome[i] = mother->chromosome[i]; // Copy genes from mother to child1 after the crossover point
            child2->chromosome[i-p-1] = father->chromosome[i]; // Copy genes from father to child2 after the crossover point
        }
        child1->crossover_place = child2->crossover_place = p; // Store the crossover point in the children
    } else {
       for(i = 0; i < chromosome_length; i++) {
            child1->chromosome[i] = father->chromosome[i]; // Copy genes from father to child1 without crossover
            child2->chromosome[i] = mother->chromosome[i]; // Copy genes from mother to child2 without crossover
        }
        child1->crossover_place = child2->crossover_place = -1; // No crossover occurred
    }
}

// Main function
int main(){
    srand((long)time(NULL)); // Seed for random number generation
    // Allocate memory for the individuals
    allocateMemory();
    // Create the first generation of individuals
    createFirstGeneration();
    // Evaluate the target function for the entire population
    evaluatePopulation(parents);
    return 0;
}