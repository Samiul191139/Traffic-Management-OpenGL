#include <GL/freeglut.h>
#include <cmath>
#include <string>
#include <sstream>


// Manual traffic signal durations
const int GREEN_DURATION = 10;
const int RED_DURATION = 10;
const int YELLOW_DURATION = 3;

// Global variables for vehicle positions
float carPosX = -0.8f;
float busPosX = 0.2f;
float bikePosY = 0.3f;

// Stop positions (just before intersection)
const float carStopX = -0.65f;
const float busStopX = -0.5f;
const float bikeStopY = 0.00f;

// Traffic signal states and timers
enum SignalState { RED, GREEN, YELLOW };
SignalState horizontalSignal = GREEN;
SignalState verticalSignal = RED;

int horizontalTimer = GREEN_DURATION;
int verticalTimer = RED_DURATION;

// Signal positions
float hSignalX = -0.3f;
float hSignalY = 0.1f;
float vSignalX = -0.5f;
float vSignalY = -0.3f;

//day-night
bool isDay = true;
// congestion
int congestionLevel = 0;

std::string feedback = "Excellent";

enum GameState { MENU, SIMULATION };
GameState currentState = MENU;

// Utility: Draw Rectangle
void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Utility: Draw Vehicle
void drawVehicle(float x, float y, float width, float height, float r, float g, float b) {
    drawRectangle(x, y, width, height, r, g, b);
}

// Utility: Draw Circle
void drawCircle(float cx, float cy, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        float x = cx + cos(angle) * radius;
        float y = cy + sin(angle) * radius;
        glVertex2f(x, y);
    }
    glEnd();
}

// Utility: Draw Timer Text
void drawText(float x, float y, std::string text) {
    glColor3f(0, 0, 0);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawText(-0.3f, 0.2f, "SMART TRAFFIC SIMULATION");
    drawText(-0.25f, 0.05f, "Press 'S' to Start Simulation");
    drawText(-0.25f, -0.05f, "Press 'E' to Exit");
    glFlush();
}

// Dashed line utility
void drawDashedLine(float x1, float y1, float x2, float y2, float dashLength, float r, float g, float b) {
    glColor3f(r, g, b);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrt(dx * dx + dy * dy);
    float step = dashLength / len;
    for (float t = 0; t < 1.0f; t += step * 2) {
        float sx = x1 + dx * t;
        float sy = y1 + dy * t;
        float ex = x1 + dx * (t + step);
        float ey = y1 + dy * (t + step);
        glVertex2f(sx, sy);
        glVertex2f(ex, ey);
    }
    glEnd();
}

void drawBuildingBlock(float x, float y, float windowOffsetX, float windowOffsetY) {
    // Main building rectangle
    drawRectangle(x, y, 0.2f, 0.4f, 0.6f, 0.6f, 0.7f);

    // Windows - 3 rows x 2 cols
    float startX = x + windowOffsetX;
    float startY = y + windowOffsetY;
    float windowWidth = 0.03f;
    float windowHeight = 0.04f;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 2; col++) {
            float wx = startX + col * 0.07f;
            float wy = startY - row * 0.07f;
            drawRectangle(wx, wy, windowWidth, windowHeight, 0.9f, 0.9f, 1.0f); // Light blue windows
        }
    }
}

void drawBuildingsRow(float startX, float startY, float dx, float dy, int count, float windowOffsetX, float windowOffsetY) {
    for (int i = 0; i < count; i++) {
        drawBuildingBlock(startX + i * dx, startY + i * dy, windowOffsetX, windowOffsetY);
    }
}

// tree
void drawTree(float x, float y) {
    drawRectangle(x, y, 0.02f, 0.08f, 0.4f, 0.2f, 0.0f); // trunk
    drawCircle(x + 0.01f, y + 0.08f, 0.04f, 0.0f, 0.6f, 0.0f); // canopy
}


// car
void drawCar(float x, float y) {
    // Car body
    drawRectangle(x, y, 0.1f, 0.05f, 1.0f, 0.0f, 0.0f); // red body
    drawRectangle(x + 0.015f, y + 0.05f, 0.07f, 0.025f, 0.9f, 0.5f, 0.5f); // roof

    // Wheels
    drawCircle(x + 0.02f, y - 0.01f, 0.015f, 0, 0, 0); // left wheel
    drawCircle(x + 0.08f, y - 0.01f, 0.015f, 0, 0, 0); // right wheel

    // Headlights (only visible at night)
    if (!isDay) {
        drawCircle(x + 0.1f, y + 0.01f, 0.01f, 1.0f, 1.0f, 0.7f); // right headlight
        drawCircle(x + 0.1f, y + 0.04f, 0.01f, 1.0f, 1.0f, 0.7f); // left headlight
    }
}

// bus
void drawBus(float x, float y) {
    drawRectangle(x, y, 0.15f, 0.06f, 0.0f, 0.0f, 0.0f); // blue body

    // Windows
    drawRectangle(x + 0.01f, y + 0.03f, 0.03f, 0.02f, 1, 1, 1);
    drawRectangle(x + 0.05f, y + 0.03f, 0.03f, 0.02f, 1, 1, 1);
    drawRectangle(x + 0.09f, y + 0.03f, 0.03f, 0.02f, 1, 1, 1);

    // Wheels
    drawCircle(x + 0.03f, y - 0.01f, 0.015f, 0, 0, 0);
    drawCircle(x + 0.12f, y - 0.01f, 0.015f, 0, 0, 0);

    // Headlights at night
    if (!isDay) {
        drawCircle(x + 0.15f, y + 0.01f, 0.012f, 1.0f, 1.0f, 0.7f);
        drawCircle(x + 0.15f, y + 0.045f, 0.012f, 1.0f, 1.0f, 0.7f);
    }
}

// vertical car
void drawVerticalCar(float x, float y) {
    // Body
    drawRectangle(x - 0.025f, y, 0.05f, 0.1f, 0.0f, 1.0f, 0.0f); // green body
    drawRectangle(x - 0.02f, y + 0.08f, 0.04f, 0.02f, 0.2f, 0.2f, 0.2f); // roof

    // Headlights (night)
    if (!isDay) {
        drawCircle(x - 0.01f, y, 0.01f, 1.0f, 1.0f, 0.6f); // left beam
        drawCircle(x + 0.01f, y, 0.01f, 1.0f, 1.0f, 0.6f); // right beam
    }
}




// Main display
void display() 
{
    if (currentState == MENU) 
    {
        drawMenu();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(isDay ? 0.8f : 0.05f, isDay ? 0.9f : 0.05f, isDay ? 0.6f : 0.6f, 1.0f);
    
    // Roads
    drawRectangle(-1.0f, -0.1f, 2.0f, 0.2f, 0.3f, 0.3f, 0.3f); // horizontal
    drawRectangle(-0.6f, -1.0f, 0.2f, 2.0f, 0.3f, 0.3f, 0.3f); // vertical

    // Lane Dividers
    drawDashedLine(-1.0f, 0.0f, 1.0f, 0.0f, 0.05f, 1, 1, 1);  // Horizontal
    drawDashedLine(-0.6f + 0.1f, -1.0f, -0.6f + 0.1f, 1.0f, 0.05f, 1, 1, 1);  // Vertical

    // Stop lines
    drawRectangle(carStopX + 0.04f, -0.1f, 0.005f, 0.2f, 1, 1, 1); // Car stop line a bit to the left
    drawRectangle(busStopX - -0.1f, -0.1f, 0.005f, 0.2f, 1, 1, 1); // Bus stop line a bit to the left
    drawRectangle(-0.6f, bikeStopY + 0.13f, 0.2f, 0.005f, 1, 1, 1); // Bike

    // Additional vertical road (top-right going up)
    drawRectangle(0.6f, 0.0f, 0.2f, 1.0f, 0.3f, 0.3f, 0.3f); // vertical extension
    drawDashedLine(0.6f + 0.1f, 0.0f, 0.6f + 0.1f, 1.0f, 0.05f, 1, 1, 1); // lane divider

    // Tree rows
    drawTree(-0.85f, 0.2f);
    drawTree(-0.35f, 0.2f);
    drawTree(0.65f, -0.5f);

    // Pool behind top buildings
    drawRectangle(-0.25f, 0.25f, 0.8f, 0.3f, 0.3f, 0.6f, 0.9f); // light blue pool

    // Top row of buildings
    drawBuildingsRow(-0.2f, 0.1f, 0.35f, 0.0f, 2, 0.05f, 0.25f);
    drawTree(-0.9f, 0.1f);
    // Bottom row of buildings
    drawBuildingsRow(-0.2f, -0.9f, 0.20f, 0.0f, 3, 0.035f, 0.30f);

    // Top right side buildings (vertical)
    drawBuildingsRow(0.38f, 0.1f, 0.0f, 0.5f, 1, 0.05f, 0.25f);

    // bottom Right side buildings (vertical)
    drawBuildingsRow(0.70f, -0.49f, 0.0f, 0.5f, 1, 0.05f, 0.30f);

    // bottom Right side buildings (vertical)
    drawBuildingsRow(0.2f, -0.49f, 0.0f, 0.5f, 1, 0.05f, 0.30f);


    // Vehicles
    drawCar(carPosX, -0.05f); // Car
    drawCircle(carPosX + 0.02f, -0.06f, 0.015f, 0, 0, 0); // Car wheel front
    drawCircle(carPosX + 0.08f, -0.06f, 0.015f, 0, 0, 0); // Car wheel rear

    drawBus(busPosX, -0.05f); // Bus
    drawCircle(busPosX + 0.02f, -0.06f, 0.015f, 0, 0, 0); // Bus wheel front
    drawCircle(busPosX + 0.13f, -0.06f, 0.015f, 0, 0, 0); // Bus wheel rear

    drawVerticalCar(-0.6f+0.05f, bikePosY);
    
    //drawCircle(-0.6f+2.1f, bikePosY + 0.02f, 0.015f, 0, 0, 0); // Bike wheel bottom
    //drawCircle(-0.6f+0.018f, bikePosY + 0.02f, 0.015f, 0, 0, 0);  // Bike wheel front

    // Signal poles and heads
    drawRectangle(hSignalX - 0.01f, hSignalY, 0.02f, 0.15f, 0.1f, 0.1f, 0.1f); // H pole
    drawRectangle(hSignalX - 0.03f, hSignalY + 0.10f, 0.06f, 0.15f, 0.1f, 0.1f, 0.1f); // H box

    drawRectangle(vSignalX - 0.01f, vSignalY, 0.02f, 0.15f, 0.1f, 0.1f, 0.1f); // V pole
    drawRectangle(vSignalX - 0.03f, vSignalY + 0.10f, 0.06f, 0.15f, 0.1f, 0.1f, 0.1f); // V box

    // Horizontal Traffic Lights
    drawCircle(hSignalX, hSignalY + 0.24f, 0.015f, horizontalSignal == RED ? 1 : 0.2f, 0, 0);             // Red
    drawCircle(hSignalX, hSignalY + 0.18f, 0.015f, horizontalSignal == YELLOW ? 1 : 0.3f, horizontalSignal == YELLOW ? 1 : 0.3f, 0); // Yellow
    drawCircle(hSignalX, hSignalY + 0.12f, 0.015f, 0, horizontalSignal == GREEN ? 1 : 0.2f, 0);           // Green

    // Vertical Traffic Lights
    drawCircle(vSignalX, vSignalY + 0.24f, 0.015f, verticalSignal == RED ? 1 : 0.2f, 0, 0);             // Red
    drawCircle(vSignalX, vSignalY + 0.18f, 0.015f, verticalSignal == YELLOW ? 1 : 0.3f, verticalSignal == YELLOW ? 1 : 0.3f, 0); // Yellow
    drawCircle(vSignalX, vSignalY + 0.12f, 0.015f, 0, verticalSignal == GREEN ? 1 : 0.2f, 0);           // Green

    // Countdown Text
    drawText(hSignalX - 0.02f, hSignalY + 0.28f, std::to_string(horizontalTimer));
    drawText(vSignalX - 0.02f, vSignalY + 0.28f, std::to_string(verticalTimer));

    //red congestion bar
    float barLength = std::min(0.3f, congestionLevel / 200.0f * 0.3f);
    drawRectangle(-0.95f, 0.85f, barLength, 0.02f, 1, 0, 0); // Red bar
    drawText(-0.95f, 0.88f, "Congestion");
    drawText(-0.3f, 0.88f, "Feedback: " + feedback);

    glFlush();
    drawText(-0.3f, 0.88f, "Feedback: " + feedback);
}

const float VEHICLE_GAP = 0.12f; // Safe distance between vehicles

// Timer updates
void timer(int value) {
    float busFrontX = busPosX + 0.25f; // correct front edge of the bus
    float safeDistance = 0.18f; // safe distance between vehicles
    static int frameCount = 0;
    frameCount++;

    bool busAhead = (busPosX - carPosX) < safeDistance && busPosX > carPosX;
    // Car movement
    if (carPosX + 0.1f < carStopX) {
        // Car is before the stop line: always move
        if (!busAhead)
        {
            carPosX += 0.005f;
        }
    }
    else if (carPosX < carStopX) {
        // Car is between its front and stop line: obey signal
        if (horizontalSignal == GREEN && !busAhead)
            carPosX += 0.005f;
    }
    else {
        // Car has crossed the stop line: always move
        if (!busAhead)
        {
            carPosX += 0.005f;
        }
    }
    if (carPosX > 1.0f) carPosX = -1.0f;



    bool carAhead = (carPosX - busPosX) < safeDistance && carPosX > busPosX;

    if (busFrontX < busStopX) {
        // Bus is before the stop line
        if (!carAhead)
            busPosX += 0.004f;
    }
    else if (busFrontX < busStopX + 0.02f) {
        // Bus at stop line
        if (horizontalSignal == GREEN && !carAhead)
            busPosX += 0.004f;
    }
    else {
        // Bus has passed stop line
        if (!carAhead)
            busPosX += 0.004f;
    }
    if (busPosX > 1.0f) busPosX = -1.0f;


    if (bikePosY > bikeStopY + 0.15f) {
        // Bike is before the stop line
        bikePosY -= 0.006f;
    }
    else if (bikePosY > bikeStopY) {
        // Bike is near the stop line: obey signal
        if (verticalSignal == GREEN)
            bikePosY -= 0.006f;
    }
    else {
        // Bike has crossed the stop line
        bikePosY -= 0.006f;
    }
    if (bikePosY < -1.0f) bikePosY = 1.0f;




    // Signal timer update every ~1 sec
    if (frameCount >= 60) {
        frameCount = 0;
        horizontalTimer--;
        verticalTimer--;

        // Horizontal signal state change
        if (horizontalTimer <= 0) {
            if (horizontalSignal == GREEN) {
                horizontalSignal = YELLOW;
                horizontalTimer = YELLOW_DURATION;
            }
            else if (horizontalSignal == YELLOW) {
                horizontalSignal = RED;
                horizontalTimer = RED_DURATION;
            }
            else {
                horizontalSignal = GREEN;
                horizontalTimer = GREEN_DURATION;
            }
        }

        // Vertical signal state change
        if (verticalTimer <= 0) {
            if (verticalSignal == RED) {
                verticalSignal = GREEN;
                verticalTimer = GREEN_DURATION;
            }
            else if (verticalSignal == GREEN) {
                verticalSignal = YELLOW;
                verticalTimer = YELLOW_DURATION;
            }
            else {
                verticalSignal = RED;
                verticalTimer = RED_DURATION;
            }
        }
    }

    int waitingCars = 0;

    // Car stopped at red/yellow light
    if ((horizontalSignal != GREEN) && (carPosX + 0.1f >= carStopX - 0.01f && carPosX <= carStopX + 0.01f)) {
        waitingCars++;
    }

    // Bus stopped at red/yellow light
    if ((horizontalSignal != GREEN) && (busFrontX >= busStopX - 0.01f && busFrontX <= busStopX + 0.01f)) {
        waitingCars++;
    }

    // Bike stopped at red/yellow light
    if ((verticalSignal != GREEN) && (bikePosY >= bikeStopY - 0.01f && bikePosY <= bikeStopY + 0.01f)) {
        waitingCars++;
    }


    if (waitingCars > 0) {
        congestionLevel += waitingCars*2;
    }
    else if (congestionLevel > 0) {
        congestionLevel -= 1; // slowly decrease
    }
    if (congestionLevel > 200) congestionLevel = 200; // cap to avoid overflow
    if (congestionLevel < 50) feedback = "Excellent";
    else if (congestionLevel < 120) feedback = "Moderate";
    else feedback = "Needs Improvement";


    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

//kyboard input
void keyboard(unsigned char key, int x, int y) {
    if (currentState == MENU) {
        if (key == 's' || key == 'S') {
            currentState = SIMULATION;
        }
        else if (key == 'e' || key == 'E') {
            exit(0);
        }
        glutPostRedisplay();
        return;
    }

    // Horizontal Signal Control
    if (key == '1') {
        horizontalSignal = GREEN;
        horizontalTimer = GREEN_DURATION;
    }
    else if (key == '2') {
        horizontalSignal = YELLOW;
        horizontalTimer = YELLOW_DURATION;
    }
    else if (key == '3') {
        horizontalSignal = RED;
        horizontalTimer = RED_DURATION;
    }

    // Vertical Signal Control
    else if (key == 'g' || key == 'G') {
        verticalSignal = GREEN;
        verticalTimer = GREEN_DURATION;
    }
    else if (key == 'h' || key == 'H') {
        verticalSignal = YELLOW;
        verticalTimer = YELLOW_DURATION;
    }
    else if (key == 'j' || key == 'J') {
        verticalSignal = RED;
        verticalTimer = RED_DURATION;
    }

    // Day/Night Toggle
    else if (key == 'd' || key == 'D') {
        isDay = true;
    }
    else if (key == 'n' || key == 'N') {
        isDay = false;
    }

    glutPostRedisplay();
}


// Entry
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Smart Traffic Simulation");
    glutKeyboardFunc(keyboard);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glutMainLoop();
    return 0;
}
