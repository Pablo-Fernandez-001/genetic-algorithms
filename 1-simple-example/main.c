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

// Global variables for the populations, roulette wheel selection, and best individual
Individual* parents;
Individual* offspring;
Individual best_individual; // Its an struct of type Individual, to store the best individual of the population
double* roulette;

// Util variables
unsigned chromosome_length;
double crossover_probability;
double mutation_probability;
unsigned generation_count;
unsigned selected_father, selected_mother; // Variables to store the selected parents for crossover


// Function to get the parameters from the user
void getParameters() {
    /*
       Here we can add the random seed, and the population size if we have our own functions
       in this case we're using the rand() function from the stdlib.h library, and the population 
       size is fixed to 100 individuals, a constant.
    */

    printf("Enter the crossover probability (0.0 - 1.0): ");
    scanf("%lf", &crossover_probability);
    printf("Enter the mutation probability (0.0 - 1.0): ");
    scanf("%lf", &mutation_probability);
    printf("Enter the max number of generations: ");
    scanf("%u", &generation_count);
}


// Function to serve the memory for the individuals
void allocateMemory() {
    unsigned required_bytes = POPULATION_SIZE * sizeof(Individual);

    parents = (Individual*)malloc(required_bytes);
    offspring = (Individual*)malloc(required_bytes);

    // Chromosome allocation for each individual
    chromosome_length = ceil(log2((FX_UPPER_BOUND - FX_LOZER_BOUND) * pow(10, PRECISION))); // Ceil function to the next nearest integer

    // We change malloc to calloc to initialize the memory to zero, and avoid garbage values in the chromosome
    // calloc initializes the allocated memory to zero, while malloc does not initialize the memory, leaving it 
    // with garbage values
    for (int i = 0; i < POPULATION_SIZE; i++) {
        parents[i].chromosome = (int*) calloc(chromosome_length, sizeof(int)); // Chromosome allocation for each individual parent
        offspring[i].chromosome = (int*) calloc(chromosome_length, sizeof(int)); // Chromosome allocation for each individual offspring
        parents[i].x = offspring[i].x = RAND_MAX; // Randomly assigning a value to x
        parents[i].fitness = offspring[i].fitness = 0; // Randomly assigning a value to fitness, the lowest value is 0 and the highest is 1
        parents[i].parents[0] = offspring[i].parents[0] = -1; // No parents for the first generation in a double array of size 2, and equally initialized to -1
        parents[i].parents[1] = offspring[i].parents[1] = -1; // No parents for the first generation in a double array of size 2, and equally initialized to -1
        parents[i].crossover_place = offspring[i].crossover_place = -1; // No crossover for the first generation, and equally initialized to -1
        parents[i].mutation_place = offspring[i].mutation_place = -1; // No mutation for the first generation, and equally initialized to -1
    }

    best_individual.chromosome = (int*) calloc(chromosome_length, sizeof(int)); // Chromosome allocation for the best individual
    best_individual.fitness = 0; // Randomly assigning a value to fitness, the lowest value is 0 and the highest is 1

    roulette = (double*) malloc(POPULATION_SIZE * sizeof(double)); // Roulette wheel selection probabilities allocation
}


//random number generator function from an interval [a, b]
double randomDouble(double a, double b) {
    return (b-a) * ((double)rand() / RAND_MAX) + a;
}


// Flip a coin function to get a random value of 0 or 1 based on a given probability
int flip(double probability) {
    return (randomDouble(0, 1) <= probability) ? 1 : 0;
}


// creating the populations
void createFirstGeneration(){
    for(int i = 0; i < POPULATION_SIZE; i++){
        for(unsigned j = 0; j < chromosome_length; j++){
            parents[i].chromosome[j] = flip(0.5); // Randomly assigning 0 or 1 to each gene in the chromosome
        }
    }
}


//convert binary to decimal function
double binary2real(int* chromosome) {
    double aux = 0.0;
    for(int i = chromosome_length - 1; i >= 0; i--){
        if(chromosome[i] == 1){
            aux += pow(2, chromosome_length - i - 1); // Convert binary to decimal
        }
    }
    return FX_LOZER_BOUND + ((aux * (FX_UPPER_BOUND - FX_LOZER_BOUND)) / (pow(2, chromosome_length) - 1));
}


// Evaluate the target function for an individual
void evaluateTargetFunction(Individual* individual) {
    individual->x = binary2real(individual->chromosome);
    individual->fitness = 1/(pow(individual->x, 2)+0.001); // Example target function: f(x) = x^2, and minimizing
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
    int current_individual;

    // the worst probability to have could be 1 
    for (int i = 0; sum < r; i++) {
        current_individual = i % POPULATION_SIZE; // Wrap around if we exceed the population size
        sum += roulette[current_individual];
    }

    return current_individual;
}


// Perform crossover between two parents to produce two children maybe with an error in p+i
void crossover(Individual* father, Individual* mother, Individual* child1, Individual* child2) {
    
    unsigned i = 0;

    if(flip(crossover_probability)) {
        // Randomly select a crossover point
        unsigned p = (unsigned)randomDouble(1, chromosome_length - 2); // Randomly select a crossover point

        // Copy genes from parents to children based on the crossover point
        for(i = 0; i <= p; i++) {
            child1->chromosome[i] = father->chromosome[i]; // Copy genes from father to child1 up to the crossover point
            child2->chromosome[p+i] = mother->chromosome[i]; // Copy genes from mother to child2 up to the crossover point
        }

        // Copy the remaining genes from the other parent to the children after the crossover point
        for(i = p+1; i < chromosome_length; i++) {
            child1->chromosome[i] = mother->chromosome[i]; // Copy genes from mother to child1 after the crossover point
            child2->chromosome[i-p-1] = father->chromosome[i]; // Copy genes from father to child2 after the crossover point
        }

        child1->crossover_place = child2->crossover_place = p; // Store the crossover point in the children
        child1->parents[0] = child2->parents[0] = selected_father + 1; // Store the index of the father in the children
        child1->parents[1] = child2->parents[1] = selected_mother + 1; // Store the index of the mother in the children

    } else {
        for(i = 0; i < chromosome_length; i++) {
            child1->chromosome[i] = father->chromosome[i]; // Copy genes from father to child1 without crossover
            child2->chromosome[i] = mother->chromosome[i]; // Copy genes from mother to child2 without crossover
        }

        child1->crossover_place = child2->crossover_place = -1; // No crossover occurred
        child1->parents[0] = child2->parents[0] = 0; // Store the index of the father in the children if the crossover does not occur
        child1->parents[1] = child2->parents[1] = 0; // Store the index of the mother in the children if the crossover does not occur
    }
}


/* Fixed code?

// Perform crossover between two parents to produce two children
void crossover(Individual* father, Individual* mother, Individual* child1, Individual* child2) {
    unsigned i = 0;

    if(flip(crossover_probability)) {
        // Randomly select a crossover point between 1 and chromosome_length - 1
        unsigned p = (unsigned)randomDouble(1, chromosome_length - 1); 
        
        // 1. Copy genes BEFORE the crossover point 'p'
        for(i = 0; i < p; i++) {
            child1->chromosome[i] = father->chromosome[i]; // Child 1 gets Father's head
            child2->chromosome[i] = mother->chromosome[i]; // Child 2 gets Mother's head
        }
        
        // 2. Swap and copy genes FROM 'p' to the end of the chromosome
        for(i = p; i < chromosome_length; i++) {
            child1->chromosome[i] = mother->chromosome[i]; // Child 1 gets Mother's tail
            child2->chromosome[i] = father->chromosome[i]; // Child 2 gets Father's tail
        }

        child1->crossover_place = child2->crossover_place = p;

    } else {
        // If no crossover occurs, clone parents directly
        for(i = 0; i < chromosome_length; i++) {
            child1->chromosome[i] = father->chromosome[i];
            child2->chromosome[i] = mother->chromosome[i];
        }

        child1->crossover_place = child2->crossover_place = -1;
    }
}

*/


// Perform mutation on an individual
void mutation(Individual* individual){
    // if the mutation occurs based on the mutation probability, flip a random bit in the chromosome
    if(flip(mutation_probability)) {
        // Randomly select a mutation place
        unsigned p = (unsigned)randomDouble(0, chromosome_length - 1); // in this case we can use 0

        // Flip the bit at the mutation point
        individual->chromosome[p] = 1 - individual->chromosome[p]; // Flip the bit at the mutation point
        individual->mutation_place = p; // Store the mutation point in the individual

    } else {
        individual->mutation_place = -1; // No mutation occurred
    }
}


// Implement elitism by replacing the worst individuals in the offspring with the best individual from the parents
// Most addapdet will survive to the next generation, and the worst will be replaced by the best of the previous generation
void elitism(){
    // The 2 worst individuals are replaced by the 2 best individuals of the previous generation
    unsigned best_parent = 0;
    unsigned worst_child1 = 0, worst_child2 = 0;
    
    // A struct if both have the same memory, and the same order you can only equalize the first one, and the second one will be equal too
    // best_individual = parents[0];

    for(int i = 0; i < POPULATION_SIZE; i++){
        if(offspring[i].fitness < offspring[worst_child1].fitness){
            worst_child1 = i;

        }else if(offspring[i].fitness < offspring[worst_child2].fitness){
            worst_child2 = i;
        }

        if(parents[i].fitness > parents[best_parent].fitness){
            best_parent = i;
        }
    }

    // Replace the worst individuals in the offspring with the best individual from the parents
    offspring[worst_child1] = parents[best_parent]; // assigning the best individual from the parents to the worst individual in the offspring
    offspring[worst_child2] = parents[best_parent]; // assigning the best individual from the parents to the worst individual in the offspring
}


//Print a cromosome with indicators
void printChromosome(Individual* individual)
{
    unsigned i;

    for(i = 0; i < chromosome_length; i++) {
        if((int)i == individual->mutation_place) printf("(");
        printf("%d", individual->chromosome[i]);
        if((int)i == individual->mutation_place) printf(")");
        if((int)i == individual->crossover_place) printf("/");
    }
}


// A Population Information
void printPopulationDetail(Individual* population)
{
    int i, current_best = 0;
    double fitness_average = 0.0;

    printf("\n\n------------------------------------------------------------\n");
    printf(" #\tChromosome\tx\tFitness\tParents");
    printf("\n------------------------------------------------------------\n");

    for(i = 0; i < POPULATION_SIZE; i++) {
        printf("\n%03d  ", i + 1);
        printChromosome(&population[i]);

        printf(" %.3f\t%.3f\t(%d,%d)",
               population[i].x,
               population[i].fitness,
               population[i].parents[0],
               population[i].parents[1]);

        fitness_average += population[i].fitness;

        if(population[i].fitness > population[current_best].fitness)
            current_best = i;

        if(population[current_best].fitness > best_individual.fitness)
            best_individual = population[i];
    }

    fitness_average /= POPULATION_SIZE;

    printf("\n------------------------------------------------------------\n");
    printf("\nAverage fitness: %.3f\n", fitness_average);
    printf("\nBest fitness: %.3f\n", population[current_best].fitness);
}


// Main function
int main()
{
    // Preparation
    getParameters();
    srand((long)time(NULL));
    allocateMemory();
    createFirstGeneration();
    evaluatePopulation(parents);

    Individual* temp_helper;
    unsigned generation;
    int i;

    for(generation = 0; generation < generation_count; generation++) {

        updateRoulette(parents);
        printPopulationDetail(parents);

        // Generation process
        for(i = 0; i < POPULATION_SIZE - 1; i += 2) {

            // Select parents using roulette wheel selection
            selected_father = rouletteWheelSelection();
            selected_mother = rouletteWheelSelection();

            // Perform crossover
            crossover(
                &parents[selected_father],
                &parents[selected_mother],
                &offspring[i],
                &offspring[i + 1]
            );

            // Perform mutation
            mutation(&offspring[i]);
            mutation(&offspring[i + 1]);

            // Evaluate offspring
            evaluateTargetFunction(&offspring[i]);
            evaluateTargetFunction(&offspring[i + 1]);
        }

        // Apply elitism
        elitism();

        // Swap populations
        temp_helper = parents;
        parents = offspring;
        offspring = temp_helper;

        printf("\n\n\tGeneration %u completed successfully\n", generation + 1);
    }

    // ---------------------- ADDED ----------------------
    printf("\n\n************************************************************");
    printf("\n\t+\tTHE BEST OF ALL");
    printf("\n************************************************************");
    printf("\n\tBinary chromosome: "); printChromosome(&best_individual);
    printf("\n\tx = %.3f\tFitness = %.3f", best_individual.x, best_individual.fitness);
    printf("\n\tParents: (%d, %d)\n", best_individual.parents[0], best_individual.parents[1]);

    free(parents);
    free(offspring);
    free(roulette);

    return 0;
}