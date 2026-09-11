/*
 * f(x1, x2, x3, x4) = (pow(M_PI, x1 - x2) / (x3 * x4)) * sin(sqrt(fabs((11*pow(x4, 2) + 13*pow(x3, 3)) / (1 + 17*x1)))) + pow(3*x1 + 5*x2 + 7*x3, 19) - exp((x1 + x3) / (x2 - x4))
 *
 * Subject to:
 * -50 <= xi <= 50,  i = 1, ..., 4
 *
 * Parameters:
 * - Real representation.
 * - Binary tournament selection.
 * - Intermediate crossover.
 * - Non-uniform mutation.
 *
 * Must to add Crossover point, and the mutation point
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Define PI if it is not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants
#define POPULATION_SIZE 50
#define FX_LOWER_BOUND -50
#define FX_UPPER_BOUND 50
#define CHROMOSOME_LENGTH 4

// Individual structure
typedef struct ind_t {
    double *chromosome; // Genes
    double Fx;          // Function value result
    double fitness;     // Aptitude
    int parents[2];     // Parents of the individual

} Individual;

// Populations
Individual* parents;
Individual* offspring;
unsigned the_best_individual; // Index of the best individual in the population

// Util Variables
double crossover_probability;
double mutation_probability;
unsigned max_generations;
unsigned selected_father, selected_mother; // Variables to store the selected parents for crossover
unsigned generation;

// Algorithm params
void getParameters()
{
    printf("\nEnter the Max number of generations: ");
    scanf("%u", &max_generations);
    printf("\nEnter the crossover probability (0.0 - 1.0): ");
    scanf("%lf", &crossover_probability);
    printf("\nEnter the mutation probability (0.0 - 1.0): ");
    scanf("%lf", &mutation_probability);
}

// Random number generator function from an interval [a, b]
double randomDouble(double lb, double ub)
{
    return lb + (rand() / (RAND_MAX / (ub - lb)));
}

// Flip a coin function to get a random value of 0 or 1 based on a given probability
int flip(double probability)
{
    return (randomDouble(0, 1) <= probability) ? 1 : 0;
}

// Serve memory for each population
void memoryAllocation()
{
    unsigned required_bytes = POPULATION_SIZE * sizeof(Individual);

    parents = (Individual *)malloc(required_bytes);
    offspring = (Individual *)malloc(required_bytes);

    for (int i = 0; i < POPULATION_SIZE; i++)
    {
        parents[i].chromosome = (double *)calloc(CHROMOSOME_LENGTH, sizeof(double));
        offspring[i].chromosome = (double *)calloc(CHROMOSOME_LENGTH, sizeof(double));
    }
}

// Initialize the first generation with random values within the specified bounds
void createFirstGeneration()
{
    for (int i = 0; i < POPULATION_SIZE; i++)
    {
        for (int j = 0; j < CHROMOSOME_LENGTH; j++)
        {
            parents[i].chromosome[j] = randomDouble(FX_LOWER_BOUND, FX_UPPER_BOUND);
        }
        parents[i].fitness = offspring[i].fitness = 0;         // Initialize fitness to 0
        parents[i].parents[0] = parents[i].parents[1] = -1;    // No parents for the first generation
        offspring[i].parents[0] = offspring[i].parents[1] = 0; // Initialize offspring parents to 0
        parents[i].Fx = offspring[i].Fx = RAND_MAX;            // Initialize function value to a large number
    }
    the_best_individual = 0; // Initialize the index of the best individual
}

// Evaluates the objective function for each individual in the population and calculates their fitness
void evaluateTargetFunction(Individual* individual){
    double x1 = individual->chromosome[0];
    double x2 = individual->chromosome[1];
    double x3 = individual->chromosome[2];
    double x4 = individual->chromosome[3];

    // Exrpession
    // We're going to split in three parts
    double expression1 = (pow(M_PI, x1 - x2)/(x3*x4)) * sin(sqrt(fabs((11*pow(x4,2)+13*pow(x3,3))/(1+17*x1))));
    double expression2 = pow(3*x1 + 5*x2 + 7*x3, 19);
    double expression3 = exp((x1 + x3)/(x2 - x4));

    // Final result
    individual->Fx = expression1 + expression2 - expression3;
}

// Evaluates a full polulation
void evaluatePopulation(Individual* population){
    for(int i = 0; i < POPULATION_SIZE; i++){
        evaluateTargetFunction(&population[i]);
    }
}

// assign fitnes value
void computeFitness(Individual* population){
    
    double min = RAND_MAX;
    double max = -RAND_MAX;

    for(int i = 0; i < POPULATION_SIZE; i++){
        if(population[i].Fx < min) {
            min = population[i].Fx;
        }
        if(population[i].Fx > max) {
            max = population[i].Fx;
        }
    }
    for(int i = 0; i < POPULATION_SIZE; i++){
        population[i].fitness = 1 - pow((population[i].Fx - min) / (max - min), 2); // Fitness is calculated as 1 - normalized squared value of the function result
    }
}

// Tournament binary selection
unsigned selection(){
    unsigned challenger_1 = (unsigned) floor(randomDouble(0, POPULATION_SIZE));
    unsigned challenger_2 = (unsigned) floor(randomDouble(0, POPULATION_SIZE));
    return (parents[challenger_1].fitness > parents[challenger_2].fitness) ? challenger_1 : challenger_2;
}

// Intermediate crossover
void crossover(Individual* father, Individual* mother, Individual* child1, Individual* child2) {
    if(flip(crossover_probability)) {

        unsigned k = (unsigned)randomDouble(0, CHROMOSOME_LENGTH - 1); // Randomly select a crossover point
        double a = randomDouble(0, 1); // Randomly select a weight for the intermediate crossover

        for(int i = 0; i < CHROMOSOME_LENGTH; i++) {
            child1->chromosome[i] = father->chromosome[i]; // first part of the chromosome from father
            child2->chromosome[i] = mother->chromosome[i]; // first part of the chromosome from mother
        }
        for(int i = k+1; i < CHROMOSOME_LENGTH; i++) {
            child1->chromosome[i] = a * mother->chromosome[i] + (1-a)*father->chromosome[i]; // second part of the chromosome from mother and father
            child2->chromosome[i] = a * father->chromosome[i] + (1-a)*mother->chromosome[i]; // second part of the chromosome from father and mother
        }
        child1->parents[0] = child2->parents[0] = selected_father; // Store the parents of the children
        child1->parents[1] = child2->parents[1] = selected_mother; // Store the parents of the children
    } else {
        for(int i = 0; i < CHROMOSOME_LENGTH; i++) {
            child1->chromosome[i] = father->chromosome[i]; // If no crossover occurs, clone parents directly
            child2->chromosome[i] = mother->chromosome[i]; // If no crossover occurs, clone parents directly
        }
        child1->parents[0] = child1->parents[1] = 0; // No parents for the first generation
    }
}


// Delta's function for non-uniform mutation
double delta(unsigned t, double y){
    double r = randomDouble(0, 1);
    unsigned T = max_generations;
    unsigned b = 5; // Non-uniformity parameter

    return y * (1 - pow(r, pow(1 - (t / T), b)));
}

// Non-uniform mutation
void mutation(Individual* individual){
    if(flip(mutation_probability)) {
        unsigned k = (unsigned)randomDouble(0, CHROMOSOME_LENGTH - 1); // Randomly select a mutation point
        unsigned Vk = individual-> chromosome[k]; // Get the value of the gene at the mutation point

        if(flip(0.5)) {
            individual->chromosome[k] = Vk + delta(generation, FX_UPPER_BOUND - Vk); // Mutate the gene by adding a delta value
        } else {
            individual->chromosome[k] = Vk - delta(generation, Vk - FX_LOWER_BOUND); // Mutate the gene by subtracting a delta value
        }
    }
}

// Elitism
void elitism(){
    unsigned best_parent, worst_child1, worst_child2 = 0;

    for(int i = 0; i < POPULATION_SIZE; i++){
        if(offspring[i].fitness < offspring[worst_child1].fitness){
            worst_child1 = i;
        } else if(offspring[i].fitness < offspring[worst_child2].fitness){
            worst_child2 = i;
        }

        if(parents[i].fitness > parents[best_parent].fitness){
            best_parent = i;
        }
    }

    offspring[worst_child1] = parents[best_parent]; // Replace the worst individual in the offspring with the best individual from the parents
    offspring[worst_child2] = parents[best_parent]; // Replace the second worst individual in the offspring with the best individual from the parents
    the_best_individual = worst_child1; // Attention: It's an indicator of the index 'cause before this line we maded the replacement, and this is the elitist individual

}

// Show the details of the population
void printPopulationDetail(Individual* population){
    printf("\n------------------------------------------------------------------------------------------\n");
    printf("#\tx1\tx2\tx3\tx4\tF(x1,x2,x3,x4)\tFitness\tParents");
    printf("\n------------------------------------------------------------------------------------------\n");

    for(int i = 0; i < POPULATION_SIZE; i++){
        printf("%02d", i + 1);

        for(int j = 0; j < CHROMOSOME_LENGTH; j++){
            printf("\t%+4.4lf", population[i].chromosome[j]);
        }

        printf("\t%+lf\t%lf\t(%d,%d)\n",
               population[i].Fx,
               population[i].fitness,
               population[i].parents[0],
               population[i].parents[1]);
    }
}

// Main function
int main()
{
    srand((unsigned)time(NULL));
    Individual* population_swapper = NULL;
    int i;

    getParameters();
    memoryAllocation();
    createFirstGeneration();
    evaluatePopulation(parents);

    for(generation = 0; generation < max_generations; generation++) {
        computeFitness(parents);
        printPopulationDetail(parents);

        for(i = 0; i < POPULATION_SIZE - 1; i += 2) {
            selected_father = selection();
            selected_mother = selection();

            crossover(&parents[selected_father], &parents[selected_mother], &offspring[i], &offspring[i+1]);

            mutation(&offspring[i]);
            mutation(&offspring[i+1]);

            evaluateTargetFunction(&offspring[i]);
            evaluateTargetFunction(&offspring[i+1]);
        }

        elitism();

        population_swapper = parents;
        parents = offspring;
        offspring = population_swapper;
    }

    printf("\n\n********************************************************************************\n");
    printf("*    BEST SOLUTION FOUND    ");
    printf("\n********************************************************************************\n\n");

    for(i = 0; i < CHROMOSOME_LENGTH; i++)
        printf("\tx%d=%lf", i + 1, parents[the_best_individual].chromosome[i]);

    printf("\n\tf(x1,x2,x3,x4)=%lf\n\n\n", parents[the_best_individual].Fx);

    return 0;
}