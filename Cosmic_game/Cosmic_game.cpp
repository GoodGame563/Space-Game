#include <GL/freeglut.h>
#include <math.h>
#include <stdlib.h>

const int STAR_COUNT = 180;
const int ASTEROID_COUNT = 14;
const int BULLET_COUNT = 24;

struct Star {
    float x, y, z, speed;
};

struct Asteroid {
    float x, y, z, size, speed, spin, spinSpeed;
    int alive;
};

struct Bullet {
    float x, y, z;
    int active;
};

Star stars[STAR_COUNT];
Asteroid asteroids[ASTEROID_COUNT];
Bullet bullets[BULLET_COUNT];

int seedValue = 17;
int winW = 1280;
int winH = 760;
int score = 0;
int health = 5;
int gameOver = 0;
float shipX = 0.0f;
float shipY = -0.95f;
float globalTime = 0.0f;
float shotCooldown = 0.0f;
unsigned char keys[256];
unsigned char specialKeys[256];

static float rnd(float a, float b)
{
    seedValue = seedValue * 1103515245 + 12345;
    int v = (seedValue / 65536) & 32767;
    return a + (b - a) * ((float)v / 32767.0f);
}

static float wave(float speed, float shift)
{
    return (float)sin(globalTime * speed + shift);
}

static void setMaterial(float r, float g, float b, float shine)
{
    GLfloat ambient[] = { r * 0.35f, g * 0.35f, b * 0.35f, 1.0f };
    GLfloat diffuse[] = { r, g, b, 1.0f };
    GLfloat specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat shininess[] = { 6.0f };
    GLfloat emission[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

static void setNeon(float r, float g, float b, float power)
{
    GLfloat ambient[] = { r * 0.20f, g * 0.20f, b * 0.20f, 1.0f };
    GLfloat diffuse[] = { r, g, b, 1.0f };
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 90.0f };
    GLfloat emission[] = { r * power, g * power, b * power, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
}

static void cube(float x, float y, float z, float sx, float sy, float sz)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

static void quad2(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0.0f);
    glVertex3f(x + w, y, 0.0f);
    glVertex3f(x + w, y + h, 0.0f);
    glVertex3f(x, y + h, 0.0f);
    glEnd();
}

static const float SHIP_SCALE = 1.0f;
static const float SHIP_HALF_WIDTH = 1.12f;
static const float SHIP_HALF_HEIGHT = 0.40f;

static float shipHalfWidth()
{
    return SHIP_HALF_WIDTH * SHIP_SCALE;
}

static float shipHalfHeight()
{
    return SHIP_HALF_HEIGHT * SHIP_SCALE;
}

static int asteroidHitsShip(const Asteroid* a)
{
    float dx = shipX - a->x;
    float dy = shipY - a->y;
    float asteroidHalf = a->size + 0.18f;
    float rx = shipHalfWidth() + asteroidHalf;
    float ry = shipHalfHeight() + asteroidHalf * 0.72f;
    return (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) < 1.0f;
}

static void drawEngineFlame(float x, float y, float z, float length, float radius, float flicker)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    setNeon(0.0f, 0.95f, 1.0f, 0.92f + 0.10f * flicker);
    glutSolidSphere(radius * 0.56f, 16, 16);

    setNeon(0.0f, 0.84f, 1.0f, 0.28f + 0.07f * flicker);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, length * 0.18f);
    glScalef(0.82f, 0.82f, 1.0f);
    glutSolidCone(radius * 1.25f, length * 0.58f, 14, 5);
    glPopMatrix();

    setNeon(1.0f, 0.44f, 0.08f, 0.34f + 0.10f * flicker);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, length * 0.34f);
    glScalef(0.92f, 0.92f, 1.0f);
    glutSolidCone(radius * 1.75f, length * 0.90f, 14, 5);
    glPopMatrix();

    setNeon(1.0f, 0.78f, 0.18f, 0.20f + 0.06f * flicker);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, length * 0.06f);
    glutSolidSphere(radius * 0.88f, 12, 12);
    glPopMatrix();

    glPopMatrix();
}

static void drawWingAssembly(float side, float lift)
{
    glPushMatrix();
    glTranslatef(0.34f * side, 0.10f * lift, 0.06f);
    glRotatef(26.0f * side * lift, 0.0f, 0.0f, 1.0f);
    glRotatef(6.0f * lift, 1.0f, 0.0f, 0.0f);

    cube(0.08f * side, 0.00f, 0.01f, 0.72f, 0.035f, 0.10f);
    cube(0.00f, 0.00f, 0.05f, 0.38f, 0.035f, 0.08f);

    cube(0.16f * side, 0.00f, 0.08f, 0.28f, 0.025f, 0.06f);
    cube(-0.02f * side, 0.00f, 0.12f, 0.22f, 0.025f, 0.08f);

    cube(0.00f, 0.00f, 0.06f, 0.40f, 0.015f, 0.025f);
    cube(0.00f, 0.00f, 0.12f, 0.14f, 0.015f, 0.020f);

    cube(0.34f * side, 0.00f, 0.04f, 0.08f, 0.020f, 0.08f);

    glPopMatrix();
}

static void drawThrusterPod(float side, float flicker)
{
    glPushMatrix();
    glTranslatef(0.36f * side, -0.03f, 0.42f);

    setMaterial(0.20f, 0.20f, 0.21f, 8.0f);
    cube(0.0f, 0.0f, -0.02f, 0.16f, 0.08f, 0.24f);
    cube(0.0f, 0.00f, 0.14f, 0.12f, 0.06f, 0.12f);

    setMaterial(0.16f, 0.11f, 0.16f, 15.0f);
    cube(0.0f, 0.03f, 0.04f, 0.10f, 0.04f, 0.10f);

    setNeon(0.18f, 0.14f, 0.18f, 0.04f + 0.01f * flicker);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.21f);
    glutSolidTorus(0.022, 0.072, 8, 16);
    glPopMatrix();

    drawEngineFlame(0.0f, -0.005f, 0.28f, 0.36f + 0.05f * flicker, 0.085f, flicker);
    glPopMatrix();
}

static void initStars()
{
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x = rnd(-8.0f, 8.0f);
        stars[i].y = rnd(-3.2f, 4.5f);
        stars[i].z = rnd(-42.0f, 2.5f);
        stars[i].speed = rnd(0.12f, 0.38f);
    }
}

static void resetAsteroid(int i, float farZ)
{
    asteroids[i].x = rnd(-3.2f, 3.2f);
    asteroids[i].y = rnd(-1.55f, 1.75f);
    asteroids[i].z = farZ + rnd(-16.0f, 0.0f);
    asteroids[i].size = rnd(0.20f, 0.55f);
    asteroids[i].speed = rnd(0.12f, 0.28f);
    asteroids[i].spin = rnd(0.0f, 360.0f);
    asteroids[i].spinSpeed = rnd(1.0f, 3.5f);
    asteroids[i].alive = 1;
}

static void initAsteroids()
{
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        resetAsteroid(i, -8.0f - (float)i * 1.8f);
    }
}

static void resetGame()
{
    seedValue = 17;
    score = 0;
    health = 5;
    gameOver = 0;
    shipX = 0.0f;
    shipY = -0.95f;
    shotCooldown = 0.0f;
    for (int i = 0; i < BULLET_COUNT; i++) {
        bullets[i].active = 0;
    }
    initStars();
    initAsteroids();
}

static void setupLights()
{
    GLfloat cyan[] = { 0.0f, 0.85f, 1.0f, 1.0f };
    GLfloat pink[] = { 1.0f, 0.08f, 0.70f, 1.0f };
    GLfloat white[] = { 0.90f, 0.95f, 1.0f, 1.0f };
    GLfloat ambient[] = { 0.05f, 0.04f, 0.12f, 1.0f };
    GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat p0[] = { -3.0f, 2.0f, 1.5f, 1.0f };
    GLfloat p1[] = { 3.0f, -0.3f, -6.0f, 1.0f };
    GLfloat p2[] = { shipX, shipY + 0.1f, 1.4f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, cyan);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT0, GL_POSITION, p0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, pink);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT1, GL_POSITION, p1);

    glLightfv(GL_LIGHT2, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT2, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT2, GL_POSITION, p2);
}

static void drawStars()
{
    glDisable(GL_LIGHTING);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < STAR_COUNT; i++) {
        float c = 0.45f + 0.55f * (float)i / (float)STAR_COUNT;
        if (i % 3 == 0) glColor3f(0.0f, 0.85f * c, 1.0f);
        if (i % 3 == 1) glColor3f(1.0f * c, 0.10f, 0.70f * c);
        if (i % 3 == 2) glColor3f(0.9f * c, 0.9f * c, 1.0f);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

static void drawTunnel()
{
    glDisable(GL_LIGHTING);

    glLineWidth(2);
    for (int r = 0; r < 10; r++) {
        float z = 2.0f - r * 2.4f + (float)fmod(globalTime * 4.0f, 2.4f);
        float size = 1.2f + (2.0f - z) * 0.11f;
        if (z > 2.5f) z -= 28.8f;

        glColor3f(0.0f, 0.42f + 0.04f * r, 0.95f);
        glBegin(GL_LINES);
        glVertex3f(-size, -size, z);
        glVertex3f(size, -size, z);
        glVertex3f(size, -size, z);
        glVertex3f(size, size, z);
        glVertex3f(size, size, z);
        glVertex3f(-size, size, z);
        glVertex3f(-size, size, z);
        glVertex3f(-size, -size, z);
        glEnd();
    }
    glLineWidth(3);
    glColor3f(1.0f, 0.05f, 0.70f);
    glBegin(GL_LINES);
    glVertex3f(-4.2f, -2.3f, 2.2f);
    glVertex3f(-0.6f, -0.2f, -28.0f);
    glVertex3f(4.2f, -2.3f, 2.2f);
    glVertex3f(0.6f, -0.2f, -28.0f);
    glVertex3f(-4.2f, 2.5f, 2.2f);
    glVertex3f(-0.6f, 0.2f, -28.0f);
    glVertex3f(4.2f, 2.5f, 2.2f);
    glVertex3f(0.6f, 0.2f, -28.0f);
    glEnd();

    glEnable(GL_LIGHTING);
}

static void drawShip()
{
    glPushMatrix();
    glTranslatef(shipX, shipY, 1.05f);
    glRotatef(shipX * -5.0f, 0.0f, 0.0f, 1.0f);
    glScalef(SHIP_SCALE, SHIP_SCALE, SHIP_SCALE);

    setMaterial(0.05f, 0.05f, 0.05f, 8.0f);
    cube(0.0f, -0.10f, -0.06f, 0.34f, 0.14f, 1.06f);
    cube(0.0f, -0.03f, 0.10f, 0.24f, 0.18f, 0.76f);

    cube(0.0f, 0.02f, 0.10f, 0.26f, 0.14f, 0.48f);
    cube(0.0f, 0.10f, 0.30f, 0.18f, 0.10f, 0.24f);
    cube(0.0f, -0.10f, 0.10f, 0.18f, 0.08f, 0.30f);

    cube(0.0f, 0.18f, -0.04f, 0.16f, 0.08f, 0.30f);
    cube(0.0f, 0.20f, 0.28f, 0.14f, 0.10f, 0.20f);


    cube(0.0f, 0.00f, 0.26f, 0.18f, 0.08f, 0.38f);
    cube(0.0f, 0.00f, 0.56f, 0.10f, 0.06f, 0.18f);


    cube(0.0f, 0.12f, 0.50f, 0.10f, 0.06f, 0.24f);
    cube(0.0f, -0.02f, 0.42f, 0.12f, 0.06f, 0.24f);

    drawWingAssembly(-1.0f, 1.0f);
    drawWingAssembly(1.0f, 1.0f);
    drawWingAssembly(-1.0f, -1.0f);
    drawWingAssembly(1.0f, -1.0f);

    cube(0.0f, 0.03f, -0.54f, 0.10f, 0.04f, 0.16f);
    glPushMatrix();
    glTranslatef(0.0f, 0.06f, 0.0f);
    glutSolidSphere(0.12, 16, 16);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 0.18f, 0.22f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.018, 0.09, 10, 18);
    glPopMatrix();


    cube(0.0f, 0.01f, 0.58f, 0.12f, 0.04f, 0.10f);
    cube(-0.10f, 0.12f, 0.38f, 0.08f, 0.03f, 0.08f);
    cube(0.10f, 0.12f, 0.38f, 0.08f, 0.03f, 0.08f);


    drawEngineFlame(0.0f, -0.01f, 0.70f, 0.60f + 0.06f * wave(14.0f, 0.4f), 0.12f, wave(16.0f, 0.0f));

    glPopMatrix();
}

static void drawAsteroid(Asteroid* a)
{
    glPushMatrix();
    glTranslatef(a->x, a->y, a->z);
    glRotatef(a->spin, 1.0f, 0.6f, 0.2f);
    glScalef(a->size * 1.15f, a->size * 0.85f, a->size);
    setMaterial(0.36f, 0.28f, 0.34f, 35.0f);
    glutSolidSphere(1.0, 18, 14);
    setMaterial(0.16f, 0.11f, 0.16f, 15.0f);
    glPushMatrix();
    glTranslatef(0.38f, 0.34f, 0.50f);
    glutSolidSphere(0.18, 10, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.40f, -0.22f, 0.54f);
    glutSolidSphere(0.13, 10, 8);
    glPopMatrix();
    glPopMatrix();
}

static void drawBullets()
{
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) continue;
        glPushMatrix();
        glTranslatef(bullets[i].x, bullets[i].y, bullets[i].z);
        setNeon(0.0f, 0.95f, 1.0f, 0.75f);
        glutSolidSphere(0.075, 16, 16);
        setNeon(1.0f, 0.08f, 0.70f, 0.22f);
        cube(0.0f, 0.0f, 0.22f, 0.045f, 0.045f, 0.44f);
        glPopMatrix();
    }
}

static void drawSegment(float x1, float y1, float x2, float y2)
{
    glBegin(GL_LINES);
    glVertex3f(x1, y1, 0.0f);
    glVertex3f(x2, y2, 0.0f);
    glEnd();
}

static void digit(int d, float x, float y, float s)
{
    int mask[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66,
        0x6D, 0x7D, 0x07, 0x7F, 0x6F
    };
    int m = mask[d % 10];
    float w = 18.0f * s;
    float h = 32.0f * s;
    float mid = 16.0f * s;
    if (m & 0x01) drawSegment(x, y + h, x + w, y + h);
    if (m & 0x02) drawSegment(x + w, y + mid, x + w, y + h);
    if (m & 0x04) drawSegment(x + w, y, x + w, y + mid);
    if (m & 0x08) drawSegment(x, y, x + w, y);
    if (m & 0x10) drawSegment(x, y, x, y + mid);
    if (m & 0x20) drawSegment(x, y + mid, x, y + h);
    if (m & 0x40) drawSegment(x, y + mid, x + w, y + mid);
}

static void letterS(float x, float y, float s)
{
    digit(5, x, y, s);
}

static void letterC(float x, float y, float s)
{
    float w = 18.0f * s;
    float h = 32.0f * s;
    drawSegment(x + w, y + h, x, y + h);
    drawSegment(x, y + h, x, y);
    drawSegment(x, y, x + w, y);
}

static void letterO(float x, float y, float s)
{
    digit(0, x, y, s);
}

static void letterR(float x, float y, float s)
{
    float w = 18.0f * s;
    float h = 32.0f * s;
    float mid = 16.0f * s;
    drawSegment(x, y, x, y + h);
    drawSegment(x, y + h, x + w, y + h);
    drawSegment(x + w, y + h, x + w, y + mid);
    drawSegment(x, y + mid, x + w, y + mid);
    drawSegment(x, y + mid, x + w, y);
}

static void letterE(float x, float y, float s)
{
    float w = 18.0f * s;
    float h = 32.0f * s;
    float mid = 16.0f * s;
    drawSegment(x, y, x, y + h);
    drawSegment(x, y + h, x + w, y + h);
    drawSegment(x, y + mid, x + w, y + mid);
    drawSegment(x, y, x + w, y);
}

static void letterH(float x, float y, float s)
{
    float w = 18.0f * s;
    float h = 32.0f * s;
    float mid = 16.0f * s;
    drawSegment(x, y, x, y + h);
    drawSegment(x + w, y, x + w, y + h);
    drawSegment(x, y + mid, x + w, y + mid);
}

static void letterP(float x, float y, float s)
{
    float w = 18.0f * s;
    float h = 32.0f * s;
    float mid = 16.0f * s;
    drawSegment(x, y, x, y + h);
    drawSegment(x, y + h, x + w, y + h);
    drawSegment(x + w, y + h, x + w, y + mid);
    drawSegment(x, y + mid, x + w, y + mid);
}

static void drawScoreNumber(int value, float x, float y)
{
    if (value > 99999) value = 99999;
    int divisor = 10000;
    int started = 0;
    for (int i = 0; i < 5; i++) {
        int d = value / divisor;
        if (d != 0 || started || divisor == 1) {
            digit(d, x, y, 0.9f);
            x += 24.0f;
            started = 1;
        }
        value = value % divisor;
        divisor /= 10;
    }
}

static void drawHud()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)winW, 0.0, (double)winH, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glLineWidth(4);

    glColor3f(0.0f, 0.95f, 1.0f);
    letterS(34.0f, (float)winH - 58.0f, 0.75f);
    letterC(58.0f, (float)winH - 58.0f, 0.75f);
    letterO(82.0f, (float)winH - 58.0f, 0.75f);
    letterR(106.0f, (float)winH - 58.0f, 0.75f);
    letterE(130.0f, (float)winH - 58.0f, 0.75f);

    glColor3f(1.0f, 0.08f, 0.70f);
    drawScoreNumber(score, 172.0f, (float)winH - 58.0f);

    glColor3f(0.0f, 0.95f, 1.0f);
    letterH(34.0f, 34.0f, 0.8f);
    letterP(62.0f, 34.0f, 0.8f);

    for (int i = 0; i < 5; i++) {
        if (i < health) glColor3f(1.0f, 0.08f, 0.70f);
        else glColor3f(0.18f, 0.06f, 0.15f);
        quad2(100.0f + i * 38.0f, 38.0f, 26.0f, 22.0f);
    }

    if (gameOver) {
        glLineWidth(8);
        glColor3f(1.0f, 0.05f, 0.70f);
        letterS((float)winW * 0.5f - 130.0f, (float)winH * 0.5f + 20.0f, 1.8f);
        letterC((float)winW * 0.5f - 76.0f, (float)winH * 0.5f + 20.0f, 1.8f);
        letterO((float)winW * 0.5f - 22.0f, (float)winH * 0.5f + 20.0f, 1.8f);
        letterR((float)winW * 0.5f + 32.0f, (float)winH * 0.5f + 20.0f, 1.8f);
        letterE((float)winW * 0.5f + 86.0f, (float)winH * 0.5f + 20.0f, 1.8f);
        glColor3f(0.0f, 0.95f, 1.0f);
        drawScoreNumber(score, (float)winW * 0.5f - 50.0f, (float)winH * 0.5f - 45.0f);
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void shoot()
{
    if (shotCooldown > 0.0f || gameOver) return;
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) {
            bullets[i].active = 1;
            bullets[i].x = shipX;
            bullets[i].y = shipY + 0.02f;
            bullets[i].z = 0.35f;
            shotCooldown = 0.18f;
            return;
        }
    }
}

static void updateGame()
{
    globalTime += 0.035f;
    if (shotCooldown > 0.0f) shotCooldown -= 0.035f;

    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].z += stars[i].speed;
        if (stars[i].z > 3.0f) {
            stars[i].x = rnd(-8.0f, 8.0f);
            stars[i].y = rnd(-3.2f, 4.5f);
            stars[i].z = rnd(-42.0f, -34.0f);
            stars[i].speed = rnd(0.12f, 0.38f);
        }
    }

    if (keys['a'] || keys['A'] || specialKeys[GLUT_KEY_LEFT]) shipX -= 0.085f;
    if (keys['d'] || keys['D'] || specialKeys[GLUT_KEY_RIGHT]) shipX += 0.085f;
    if (keys['w'] || keys['W'] || specialKeys[GLUT_KEY_UP]) shipY += 0.060f;
    if (keys['s'] || keys['S'] || specialKeys[GLUT_KEY_DOWN]) shipY -= 0.060f;
    if (keys[' ']) shoot();

    float shipMinX = -3.15f + shipHalfWidth();
    float shipMaxX = 3.15f - shipHalfWidth();
    float shipMinY = -1.75f + shipHalfHeight();
    float shipMaxY = 1.35f - shipHalfHeight();

    if (shipX < shipMinX) shipX = shipMinX;
    if (shipX > shipMaxX) shipX = shipMaxX;
    if (shipY < shipMinY) shipY = shipMinY;
    if (shipY > shipMaxY) shipY = shipMaxY;

    if (gameOver) return;

    for (int i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) continue;
        bullets[i].z -= 0.72f;
        if (bullets[i].z < -34.0f) bullets[i].active = 0;
    }

    for (int i = 0; i < ASTEROID_COUNT; i++) {
        asteroids[i].z += asteroids[i].speed;
        asteroids[i].spin += asteroids[i].spinSpeed;
        if (asteroids[i].spin > 360.0f) asteroids[i].spin -= 360.0f;

        for (int b = 0; b < BULLET_COUNT; b++) {
            if (!bullets[b].active) continue;
            float dx = bullets[b].x - asteroids[i].x;
            float dy = bullets[b].y - asteroids[i].y;
            float dz = bullets[b].z - asteroids[i].z;
            float dist = dx * dx + dy * dy + dz * dz;
            float radius = asteroids[i].size + 0.20f;
            if (dist < radius * radius) {
                bullets[b].active = 0;
                score += 10;
                resetAsteroid(i, -22.0f);
                break;
            }
        }

        if (asteroids[i].z > 1.55f) {
            if (asteroidHitsShip(&asteroids[i])) {
                health--;
                if (health <= 0) {
                    health = 0;
                    gameOver = 1;
                }
            }
            resetAsteroid(i, -24.0f);
        }
    }
}

static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, winW, winH);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.45, 6.2,
        0.0, 0.0, -16.0,
        0.0, 1.0, 0.0);

    setupLights();
    drawStars();
    drawTunnel();
    drawShip();


    for (int i = 0; i < ASTEROID_COUNT; i++) {
        drawAsteroid(&asteroids[i]);
    }
    drawBullets();
    drawHud();

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(58.0, (float)w / (float)h, 0.35, 60.0);
    glMatrixMode(GL_MODELVIEW);
}

static void timer(int value)
{
    updateGame();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    keys[key] = 1;
    if (key == 'r' || key == 'R' || key == 'к' || key == 'К' ) resetGame();
    if (key == 27) exit(0);
}

void keyboardUp(unsigned char key, int x, int y)
{
    keys[key] = 0;
}

void specialDown(int key, int x, int y)
{
    if (key >= 0 && key < 256) specialKeys[key] = 1;
}

void specialUp(int key, int x, int y)
{
    if (key >= 0 && key < 256) specialKeys[key] = 0;
}

void init()
{
    glClearColor(0.004f, 0.004f, 0.030f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glShadeModel(GL_SMOOTH);
    resetGame();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(58.0, (float)winW / (float)winH, 0.35, 60.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(35, 35);
    glutCreateWindow("Space Flight");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timer, 0);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutMainLoop();
    return 0;
}
