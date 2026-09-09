#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Using a population of 100 individuals
#define POPULATION_SIZE 100U
#define FX_LOWER_BOUND -20.0
#define FX_UPPER_BOUND 20.0
#define PRECISION 3

// Making an Individual like a struct
typedef struct ind_t {
    int* chromosome;
    double x; // Genes
    double fitness; // Aptitude
    int parents[2]; // Parents of the individual
    int mutation_place; // Place where the mutation happened
    int crossover_place; // Place where the crossover happened
} Individual;

// Global variables for the populations, roulette wheel selection, and best individual
Individual* parents = NULL;
Individual* offspring = NULL;
Individual best_individual = {0};
double* roulette = NULL;

// Utility variables
unsigned chromosome_length = 0U;
double crossover_probability = 0.0;
double mutation_probability = 0.0;
unsigned generation_count = 0U;
unsigned selected_father = 0U, selected_mother = 0U;

// Function to get the parameters from the user
void getParameters(void) {
    printf("Enter the crossover probability (0.0 - 1.0): ");
    if(scanf("%lf", &crossover_probability) != 1 || crossover_probability < 0.0 || crossover_probability > 1.0) {
        fprintf(stderr, "Error: crossover probability must be between 0.0 and 1.0.\n");
        exit(EXIT_FAILURE);
    }

    printf("Enter the mutation probability (0.0 - 1.0): ");
    if(scanf("%lf", &mutation_probability) != 1 || mutation_probability < 0.0 || mutation_probability > 1.0) {
        fprintf(stderr, "Error: mutation probability must be between 0.0 and 1.0.\n");
        exit(EXIT_FAILURE);
    }

    printf("Enter the max number of generations: ");
    if(scanf("%u", &generation_count) != 1 || generation_count == 0U) {
        fprintf(stderr, "Error: the number of generations must be greater than 0.\n");
        exit(EXIT_FAILURE);
    }
}

// Free all dynamically allocated memory
void freeMemory(void) {
    if(parents != NULL) {
        for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
            free(parents[i].chromosome);
        }
    }

    if(offspring != NULL) {
        for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
            free(offspring[i].chromosome);
        }
    }

    free(parents);
    free(offspring);
    free(best_individual.chromosome);
    free(roulette);

    parents = NULL;
    offspring = NULL;
    best_individual.chromosome = NULL;
    roulette = NULL;
}

// Function to allocate memory for the individuals
void allocateMemory(void) {
    double number_of_values = (FX_UPPER_BOUND - FX_LOWER_BOUND) * pow(10.0, PRECISION) + 1.0;
    chromosome_length = (unsigned)ceil(log2(number_of_values));

    if(chromosome_length < 2U) {
        fprintf(stderr, "Error: chromosome length must be at least 2.\n");
        exit(EXIT_FAILURE);
    }

    parents = (Individual*)calloc(POPULATION_SIZE, sizeof(Individual));
    offspring = (Individual*)calloc(POPULATION_SIZE, sizeof(Individual));
    roulette = (double*)malloc(POPULATION_SIZE * sizeof(double));
    best_individual.chromosome = (int*)calloc(chromosome_length, sizeof(int));

    if(parents == NULL || offspring == NULL || roulette == NULL || best_individual.chromosome == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        freeMemory();
        exit(EXIT_FAILURE);
    }

    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        parents[i].chromosome = (int*)calloc(chromosome_length, sizeof(int));
        offspring[i].chromosome = (int*)calloc(chromosome_length, sizeof(int));

        if(parents[i].chromosome == NULL || offspring[i].chromosome == NULL) {
            fprintf(stderr, "Error: chromosome memory allocation failed.\n");
            freeMemory();
            exit(EXIT_FAILURE);
        }

        parents[i].x = offspring[i].x = 0.0;
        parents[i].fitness = offspring[i].fitness = 0.0;
        parents[i].parents[0] = offspring[i].parents[0] = -1;
        parents[i].parents[1] = offspring[i].parents[1] = -1;
        parents[i].crossover_place = offspring[i].crossover_place = -1;
        parents[i].mutation_place = offspring[i].mutation_place = -1;
    }

    best_individual.x = 0.0;
    best_individual.fitness = 0.0;
    best_individual.parents[0] = -1;
    best_individual.parents[1] = -1;
    best_individual.crossover_place = -1;
    best_individual.mutation_place = -1;
}

// Random number generator function from an interval [a, b)
double randomDouble(double a, double b) {
    return a + (b - a) * ((double)rand() / ((double)RAND_MAX + 1.0));
}

// Flip a coin function to get a random value of 0 or 1 based on a given probability
int flip(double probability) {
    return (randomDouble(0.0, 1.0) < probability) ? 1 : 0;
}

// Creating the first generation
void createFirstGeneration(void) {
    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        for(unsigned j = 0U; j < chromosome_length; j++) {
            parents[i].chromosome[j] = flip(0.5);
        }
    }
}

// Convert binary to decimal function
double binary2real(const int* chromosome) {
    double decimal_value = 0.0;

    for(unsigned i = 0U; i < chromosome_length; i++) {
        decimal_value = decimal_value * 2.0 + chromosome[i];
    }

    double max_decimal_value = pow(2.0, (double)chromosome_length) - 1.0;
    return FX_LOWER_BOUND + (decimal_value * (FX_UPPER_BOUND - FX_LOWER_BOUND)) / max_decimal_value;
}

// Evaluate the target function for an individual
void evaluateTargetFunction(Individual* individual) {
    individual->x = binary2real(individual->chromosome);
    individual->fitness = 1.0 / (pow(individual->x, 2.0) + 0.001);
}

// Evaluate the target function for the entire population
void evaluatePopulation(Individual* population) {
    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        evaluateTargetFunction(&population[i]);
    }
}

// Update the roulette wheel selection probabilities based on the fitness of the individuals
void updateRoulette(const Individual* population) {
    double total_fitness = 0.0;

    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        total_fitness += population[i].fitness;
    }

    if(total_fitness <= 0.0 || !isfinite(total_fitness)) {
        for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
            roulette[i] = (double)(i + 1U) / (double)POPULATION_SIZE;
        }
        return;
    }

    double cumulative_probability = 0.0;

    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        cumulative_probability += population[i].fitness / total_fitness;
        roulette[i] = cumulative_probability;
    }

    roulette[POPULATION_SIZE - 1U] = 1.0;
}

// Making the selection of the parents based on the roulette wheel selection method
unsigned rouletteWheelSelection(void) {
    double r = randomDouble(0.0, 1.0);

    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        if(r <= roulette[i]) {
            return i;
        }
    }

    return POPULATION_SIZE - 1U;
}

// Perform crossover between two parents to produce two children
void crossover(const Individual* father, const Individual* mother, Individual* child1, Individual* child2) {
    if(flip(crossover_probability)) {
        unsigned p = (unsigned)randomDouble(0.0, (double)(chromosome_length - 1U));

        for(unsigned i = 0U; i <= p; i++) {
            child1->chromosome[i] = father->chromosome[i];
            child2->chromosome[i] = mother->chromosome[i];
        }

        for(unsigned i = p + 1U; i < chromosome_length; i++) {
            child1->chromosome[i] = mother->chromosome[i];
            child2->chromosome[i] = father->chromosome[i];
        }

        child1->crossover_place = child2->crossover_place = (int)p;

    } else {
        for(unsigned i = 0U; i < chromosome_length; i++) {
            child1->chromosome[i] = father->chromosome[i];
            child2->chromosome[i] = mother->chromosome[i];
        }

        child1->crossover_place = child2->crossover_place = -1;
    }

    child1->parents[0] = child2->parents[0] = (int)selected_father + 1;
    child1->parents[1] = child2->parents[1] = (int)selected_mother + 1;
}

// Perform mutation on an individual
void mutation(Individual* individual) {
    if(flip(mutation_probability)) {
        unsigned p = (unsigned)randomDouble(0.0, (double)chromosome_length);

        individual->chromosome[p] = 1 - individual->chromosome[p];
        individual->mutation_place = (int)p;

    } else {
        individual->mutation_place = -1;
    }
}

// Deep-copy an individual without sharing the chromosome pointer
void copyIndividual(Individual* destination, const Individual* source) {
    for(unsigned i = 0U; i < chromosome_length; i++) {
        destination->chromosome[i] = source->chromosome[i];
    }

    destination->x = source->x;
    destination->fitness = source->fitness;
    destination->parents[0] = source->parents[0];
    destination->parents[1] = source->parents[1];
    destination->mutation_place = source->mutation_place;
    destination->crossover_place = source->crossover_place;
}

// Implement elitism by replacing the two worst offspring with the best parent
void elitism(void) {
    unsigned best_parent = 0U;
    unsigned worst_child1 = 0U, worst_child2 = 1U;

    for(unsigned i = 1U; i < POPULATION_SIZE; i++) {
        if(parents[i].fitness > parents[best_parent].fitness) {
            best_parent = i;
        }
    }

    if(offspring[worst_child2].fitness < offspring[worst_child1].fitness) {
        unsigned temp = worst_child1;
        worst_child1 = worst_child2;
        worst_child2 = temp;
    }

    for(unsigned i = 2U; i < POPULATION_SIZE; i++) {
        if(offspring[i].fitness < offspring[worst_child1].fitness) {
            worst_child2 = worst_child1;
            worst_child1 = i;

        } else if(offspring[i].fitness < offspring[worst_child2].fitness) {
            worst_child2 = i;
        }
    }

    copyIndividual(&offspring[worst_child1], &parents[best_parent]);
    copyIndividual(&offspring[worst_child2], &parents[best_parent]);
}

// Print a chromosome with mutation and crossover indicators
void printChromosome(const Individual* individual) {
    for(unsigned i = 0U; i < chromosome_length; i++) {
        if((int)i == individual->mutation_place) printf("(");
        printf("%d", individual->chromosome[i]);
        if((int)i == individual->mutation_place) printf(")");
        if((int)i == individual->crossover_place) printf("/");
    }
}

// Print the information of a population
void printPopulationDetail(const Individual* population) {
    unsigned current_best = 0U;
    double fitness_average = 0.0;

    printf("\n\n------------------------------------------------------------\n");
    printf(" #\tChromosome\tx\tFitness\tParents");
    printf("\n------------------------------------------------------------\n");

    for(unsigned i = 0U; i < POPULATION_SIZE; i++) {
        printf("\n%03u  ", i + 1U);
        printChromosome(&population[i]);

        printf(" %.3f\t%.3f\t(%d,%d)",
               population[i].x,
               population[i].fitness,
               population[i].parents[0],
               population[i].parents[1]);

        fitness_average += population[i].fitness;

        if(population[i].fitness > population[current_best].fitness) {
            current_best = i;
        }
    }

    fitness_average /= (double)POPULATION_SIZE;

    if(population[current_best].fitness > best_individual.fitness) {
        copyIndividual(&best_individual, &population[current_best]);
    }

    printf("\n------------------------------------------------------------\n");
    printf("\nAverage fitness: %.3f\n", fitness_average);
    printf("Best fitness: %.3f\n", population[current_best].fitness);
}

// Main function
int main(void) {
    // Preparation
    getParameters();
    srand((unsigned)time(NULL));
    allocateMemory();
    createFirstGeneration();
    evaluatePopulation(parents);

    Individual* temp_helper = NULL;

    for(unsigned generation = 0U; generation < generation_count; generation++) {
        updateRoulette(parents);
        printPopulationDetail(parents);

        // Generation process
        for(unsigned i = 0U; i + 1U < POPULATION_SIZE; i += 2U) {
            selected_father = rouletteWheelSelection();
            selected_mother = rouletteWheelSelection();

            crossover(&parents[selected_father],
                      &parents[selected_mother],
                      &offspring[i],
                      &offspring[i + 1U]);

            mutation(&offspring[i]);
            mutation(&offspring[i + 1U]);

            evaluateTargetFunction(&offspring[i]);
            evaluateTargetFunction(&offspring[i + 1U]);
        }

        elitism();

        temp_helper = parents;
        parents = offspring;
        offspring = temp_helper;

        printf("\n\n\tGeneration %u completed successfully\n", generation + 1U);
    }

    // Show the last generated population
    printPopulationDetail(parents);

    // Show the best individual of all generations
    printf("\n\n************************************************************");
    printf("\n\t+\tTHE BEST OF ALL");
    printf("\n************************************************************");
    printf("\n\tBinary chromosome: "); printChromosome(&best_individual);
    printf("\n\tx = %.3f\tFitness = %.3f", best_individual.x, best_individual.fitness);
    printf("\n\tParents: (%d, %d)\n", best_individual.parents[0], best_individual.parents[1]);

    freeMemory();

    return EXIT_SUCCESS;
}