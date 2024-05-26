#include "header.h"
#include "argumetns.h"
typedef struct
{
    float x, y;
    float direction;
    int speed;
    int pheromone; // 1-10
    float size;
    boolean sleep;          // for eating
    clock_t lastUpdateTime; // for rotating when it smells a pheromone
    int foodIndex;
    int id;
} Ant;

typedef struct
{
    float x, y;
    float amount; // 100
    pthread_mutex_t mutex;
} Food;

Ant antData[200];
Food foodData[25];

pthread_t ants[100];
int ants_id[200];

int direction[] = {0, 45, 90, 135, 180, 225, 270, 315};

float getDistance(float x0, float y0, float x1, float y1)
{
    // printf("%f %f %f %f\n\n", x0, y0, x1, y1);
    return sqrt(pow((x0 - x1), 2) + pow((y0 - y1), 2));
}

void update_ant_position(Ant *antInput)
{
    int cc = rand() % 2;

    antInput->x += antInput->speed * 0.1 * cos(antInput->direction);
    antInput->y += antInput->speed * 0.1 * sin(antInput->direction);

    // Keep the ant within the window bounds
    if (antInput->x < 0)
    {
        antInput->x = 0;
        antInput->direction += (M_PI + (M_PI / 4)); // Turn 45 degrees
    }
    else if (antInput->x > window_width)
    {
        antInput->x = window_width;
        antInput->direction += (M_PI + (M_PI / 4)); // Turn 45 degrees
    }

    else if (antInput->y < 0)
    {
        antInput->y = 0;
        antInput->direction += (M_PI + (M_PI / 4)); // Turn 45 degrees
    }
    else if (antInput->y > window_height)
    {
        antInput->y = window_height;
        antInput->direction += (M_PI + (M_PI / 4)); // Turn 45 degrees
    }

    // Ensuring the direction is between 0 and 2*pi
    while (antInput->direction < 0)
        antInput->direction += 2 * M_PI;
    while (antInput->direction > 2 * M_PI)
        antInput->direction -= 2 * M_PI;
}

void update_ant_behavior(Ant *antInput)
{
    float minDistance = INFINITY;
    int closestFoodIndex = -1;

    // Find the closest food source
    for (int i = 0; i < num_food; i++)
    {
        if (foodData[i].amount > 0)
        {
            float distanceToFood = getDistance(antInput->x, antInput->y, foodData[i].x, foodData[i].y);
            if (distanceToFood < minDistance)
            {
                minDistance = distanceToFood;
                closestFoodIndex = i;
            }
        }
    }

    if (closestFoodIndex != -1 && minDistance <= food_sniffing_distance && foodData[closestFoodIndex].amount > 0)
    {
        antInput->direction = atan2(foodData[closestFoodIndex].y - antInput->y, foodData[closestFoodIndex].x - antInput->x);
        antInput->pheromone = 10; //(int)(10 * (1 - (minDistance / food_sniffing_distance)));
        fflush(stdout);
        antInput->foodIndex = closestFoodIndex;
        if (minDistance < 1)
        {
            bool enter = false;
            if (pthread_mutex_trylock(&foodData[closestFoodIndex].mutex) == 0)
            {
                if (foodData[closestFoodIndex].amount > 0)
                {
                    foodData[closestFoodIndex].amount -= food_portion;
                    if (foodData[closestFoodIndex].amount < 0)
                        foodData[closestFoodIndex].amount = 0;
                    antInput->size += 0.20;
                    enter = true;
                }
                pthread_mutex_unlock(&foodData[closestFoodIndex].mutex);
            }
            if (enter)
            {
                antInput->sleep = true;
            }
        }
    }
    else if (antInput->pheromone == 10)
    {
        antInput->pheromone = 0;
        antInput->foodIndex = -1;
    }

    // Update direction based on pheromone smell
    if (antInput->pheromone < 10)
    {
        float maxPheromone = 0;
        int closestPheromoneIndex = -1;
        for (int i = 0; i < numof_ants; i++)
        {
            if (antData[i].id != antInput->id)
            {

                float distanceToAnt = getDistance(antInput->x, antInput->y, antData[i].x, antData[i].y);
                fflush(stdout);
                if ((distanceToAnt <= (min_range_pheromone * antData[i].pheromone)) && antData[i].pheromone > antInput->pheromone && antData[i].pheromone > maxPheromone)
                {
                    maxPheromone = antData[i].pheromone;
                    closestPheromoneIndex = i;
                }
            }
        }

        clock_t currentTime = time(NULL);
        double elpasedTime = (double)(currentTime - antInput->lastUpdateTime);

        if (closestPheromoneIndex != -1 && elpasedTime >= 1)
        {
            closestFoodIndex = antData[closestPheromoneIndex].foodIndex;
            antInput->pheromone = maxPheromone > 2 ? maxPheromone - 2 : maxPheromone;
            float angleToFood = atan2(foodData[closestFoodIndex].y - antInput->y, foodData[closestFoodIndex].x - antInput->x);
            float angleDifference = angleToFood - antInput->direction;
            if (angleDifference > M_PI)
                angleDifference -= 2 * M_PI;
            else if (angleDifference < -M_PI)
                angleDifference += 2 * M_PI;
            antInput->direction += (angleDifference > 0) ? 0.261 : -0.261; // Angle adjustment of 5 degrees
            // antInput->pheromone = 10;                                    // (5*pie/180)*2
            antInput->lastUpdateTime = currentTime;
            antInput->foodIndex=closestFoodIndex; // to make the ants that sniff the food follow us
        }
        else if (closestPheromoneIndex == -1)
        {
            antInput->pheromone = 0;
        }
    }
}

void initialize_food(Food *food, int num_food)
{
    for (int i = 0; i < num_food; i++)
    {
        food[i].x = rand() % window_width;
        food[i].y = rand() % window_height;
        food[i].amount = 0; // You can adjust this value as needed
        food[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < numof_ants; i++)
    {
        if (antData[i].pheromone)
        {
            glColor3f(antData[i].pheromone * 0.05 + 0.5, 0.9f, 0.8f);
            glPushMatrix();
            glTranslatef(antData[i].x, antData[i].y, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0.0, 0.0); // Center of the circle
            for (float angle = 0.0; angle <= 2 * M_PI + 0.1; angle += 0.1)
            {
                float x = antData[i].pheromone ? min_range_pheromone * cos(angle) * antData[i].pheromone : 0;
                float y = antData[i].pheromone ? min_range_pheromone * sin(angle) * antData[i].pheromone : 0;
                glVertex2f(x, y);
            }
            glEnd();
            glPopMatrix();
        }
    }

    // Render food
    glColor3f(1.0, 0.0, 0.0);
    for (int i = 0; i < num_food; i++)
    {
        if (foodData[i].amount != 0)
        {
            glPushMatrix();
            glTranslatef(foodData[i].x, foodData[i].y, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0.0, 0.0); // Center of the circle
            for (float angle = 0.0; angle <= 2 * M_PI + 0.1; angle += 0.1)
            {
                float x = 20.0 * foodData[i].amount * 0.01 * cos(angle); // Adjust the circle size based on food amount
                float y = 20.0 * foodData[i].amount * 0.01 * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();
            glPopMatrix();
        }
    }

    // Render ants
    glColor3f(0.0, 0.0, 0.0);
    for (int i = 0; i < numof_ants; i++)
    {
        glPushMatrix();
        glTranslatef(antData[i].x, antData[i].y, 0.0);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0, 0.0); // Center of the circle
        for (float angle = 0.0; angle <= 2 * M_PI + 0.1; angle += 0.1)
        {
            float x = antData[i].size * cos(angle);
            float y = antData[i].size * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
        glPopMatrix();
    }

    glutSwapBuffers();
}

void update_simulation(int value)
{
    for (int i = 0; i < numof_ants; i++)
    {
        if (!antData[i].sleep)
        {
            update_ant_position(&antData[i]);
            update_ant_behavior(&antData[i]);
        }
    }

    // Update other simulation components

    glutPostRedisplay();
    glutTimerFunc(1000 / 60, update_simulation, 0);
}

void init_gl()
{
    glClearColor(0.97, 0.97, 0.86, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, window_width, 0, window_height);
}

// pthread_t FOOD[numof_ants];

void ant_fun(void *data)
{
    int index = *((int *)data);
    antData[index].x = rand() % 10;

    antData[index].x = rand() % window_width;
    antData[index].y = rand() % window_height;
    antData[index].direction = direction[rand() % 8] * (M_PI / 180.0);
    antData[index].speed = (rand() % 10) + 1;
    antData[index].pheromone = 0;
    antData[index].size = 2;
    antData[index].sleep = false;
    antData[index].id = index;
    antData[index].lastUpdateTime = 0;
    antData[index].foodIndex = -1;

    sleep(2);
    while (1)
    {
        if (antData[index].sleep)
        {
            antData[index].sleep = false;
            // printf("eating for %d sec...\n",eating_delay);
            sleep(eating_delay);
        }

        sleep(1);
    }
}

void add_food_source()
{
    for (int i = 0; i < num_food; i++)
    {
        if (foodData[i].amount == 0)
        { // Find an empty food source slot
            foodData[i].x = rand() % window_width;
            foodData[i].y = rand() % window_height;
            foodData[i].amount = 100; // You can adjust this value as needed
            break;
        }
    }
}

void terminate()
{
    printf("terminating...\n");
    exit(0);
}

void add_food_timer(int value)
{
    add_food_source();
    glutTimerFunc(food_delay * 1000, add_food_timer, 0);
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    FILE *f = fopen("arguments.txt", "r");

    if (f == NULL)
    {
        perror("no such file or directory");
        exit(1);
    }

    fscanf(f, "%d %*s", &numof_ants); // %*s ignores the string after the number
    fscanf(f, "%d %*s", &food_delay);
    fscanf(f, "%d %*s", &window_width);
    fscanf(f, "%d %*s", &window_height);
    fscanf(f, "%d %*s", &food_sniffing_distance);
    fscanf(f, "%d %*s", &min_range_pheromone);
    fscanf(f, "%d %*s", &num_food);
    fscanf(f, "%f %*s", &food_portion);
    fscanf(f, "%d %*s", &eating_delay);
    fscanf(f, "%d %*s", &finish_delay);

    food_portion *= 100;
    int i;
    for (i = 0; i < numof_ants; i++)
    {
        int *index = malloc(sizeof(int));
        *index = i;
        ants_id[i] = pthread_create(&ants[i], NULL, ant_fun, (void *)index);
    }

    sleep(1);

    // initialize_ants(ants, numof_ants);

    initialize_food(foodData, num_food);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Ant Simulation");
    init_gl();
    glutDisplayFunc(display);
    glutTimerFunc(1000 / 60, update_simulation, 0);
    glutTimerFunc(food_delay * 1000, add_food_timer, 0);
    glutTimerFunc(finish_delay * 1000, terminate, 0);
    glutMainLoop();
    return 0;
}