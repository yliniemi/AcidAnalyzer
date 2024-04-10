

#include <linmath.h>

#include <stdio.h>
#include <stdlib.h>

#include <drawGLFW.h>

#define GLAD_GL_IMPLEMENTATION
// #include <glad/gl.h>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
 // Make MS math.h define M_PI
 #define _USE_MATH_DEFINES
#endif

#include <OpenSimplex2F.h>
#include <nanoTime.h>
#include <time.h>
// #include <ncurses.h>
#include <drawNcurses.h>

#include <stddef.h> // Include the header file for offsetof

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif


#include <defaults.h>
extern struct Global global;

struct OpenSimplex2F_context *simplexContext;

static int64_t currentNanoTime = 0;

static double sizeRatio = 1.0;
static double xRatio = 1.0;
static double yRatio = 1.0;
static double minRatio = 1.0;

static int64_t errorX = 0;
static int64_t errorY = 0;

static int64_t sizeInt = 0;
static int64_t xSizeInt = 0;
static double emptyCircleRatio = 0.2;

static struct Line2D centralLine = {{0.5, 0}, {0.5, 1}};

static struct Vector2D offset = {0, 0};


double fmin(double a, double b)
{
    return a < b ? a : b;
}

double fmax(double a, double b)
{
    return a > b ? a : b;
}

double nextBin(double currentBin, double ratio)
{
    return floor(currentBin * ratio + 1.0);
}

double properSin(double ratioAngle)
{
    return sin(ratioAngle * 2 * PI);
}

double properCos(double ratioAngle)
{
    return cos(ratioAngle * 2 * PI);
}

double smoothStep(double x)
{
   return x * x * (3.0 - 2.0 * x);
}

double lerp(double a, double b, double f) 
{
    return (a * (1.0 - f)) + (b * f);
}

struct Vector2D lerp2d(struct Vector2D a, struct Vector2D b, double f) 
{
    struct Vector2D result;
    result.x = lerp(a.x, b.x, f);
    result.y = lerp(a.y, b.y, f);
    return result;
}

struct Vector2D add2d(struct Vector2D a, struct Vector2D b) 
{
    struct Vector2D result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

struct Vector2D substract2d(struct Vector2D a, struct Vector2D b) 
{
    struct Vector2D result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

struct Vector2D addPortion2d(struct Vector2D a, struct Vector2D b, double f) 
{
    struct Vector2D result;
    result.x = a.x + b.x * f;
    result.y = a.y + b.y * f;
    return result;
}

struct Vector3D lerp3d(struct Vector3D a, struct Vector3D b, double f) 
{
    struct Vector3D result;
    result.x = lerp(a.x, b.x, f);
    result.y = lerp(a.y, b.y, f);
    result.z = lerp(a.z, b.z, f);
    return result;
}

double smootherStep(double x)
{
   return x * x * x * (x * (6.0 * x - 15.0) + 10.0);
}

struct RGB HSVToRGB(struct HSV hsv)
{
	struct RGB rgb;
	hsv.h = fmod(hsv.h, 1.0);
	if (hsv.h < 0) hsv.h += 1;
	if (hsv.s <= 0)
	{
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
	}
	else
	{
		int i;
		double f, p, q, t;
		if (hsv.s > 1.0) hsv.s = 1.0; 

		if (hsv.h <= 0)
			hsv.h = 0;
		else
		{
			hsv.h = fmod(hsv.h, 1.0);
		}

		i = (int)trunc(hsv.h * 6.0);
		f = hsv.h * 6.0 - i;

		p = hsv.v * (1.0 - hsv.s);
		q = hsv.v * (1.0 - (hsv.s * f));
		t = hsv.v * (1.0 - (hsv.s * (1.0 - f)));

		switch (i)
		{
		case 0:
			rgb.r = hsv.v;
			rgb.g = t;
			rgb.b = p;
			break;

		case 1:
			rgb.r = q;
			rgb.g = hsv.v;
			rgb.b = p;
			break;

		case 2:
			rgb.r = p;
			rgb.g = hsv.v;
			rgb.b = t;
			break;

		case 3:
			rgb.r = p;
			rgb.g = q;
			rgb.b = hsv.v;
			break;

		case 4:
			rgb.r = t;
			rgb.g = p;
			rgb.b = hsv.v;
			break;

		default:
			rgb.r = hsv.v;
			rgb.g = p;
			rgb.b = q;
			break;
		}
	}
	return rgb;
}

void calculateWaveData(struct AllChannelData *allChannelData)
{
    int64_t secondBin = nextBin(allChannelData->firstBin, allChannelData->ratio);
    double firstBinLog = log2(sqrt(allChannelData->firstBin * (secondBin - 1)));
    double secondToLastBinLog = log2(sqrt(allChannelData->secondToLastBin * (allChannelData->lastBin - 1)));
    
    for (int64_t channel = 0; channel < allChannelData->numberOfChannels; channel++)
    {
        int64_t currentBin = allChannelData->firstBin;
        
        struct ChannelData *channelData = allChannelData->channelDataArray[channel];
        double *log10bands = channelData->log10Bands;
        for (int64_t barIndex = 0; barIndex < global.numBars; barIndex++)
        {
            int64_t nextBinValue = nextBin(currentBin, allChannelData->ratio);
            
            struct Edge *edge = &channelData->edge[barIndex + 1];
            edge->height = (log10bands[barIndex] + global.dynamicRange) / global.dynamicRange;
            edge->location = (log2(sqrt(currentBin * (nextBinValue - 1))) - firstBinLog) / (secondToLastBinLog - firstBinLog);
            
            if (edge->height < global.minBarHeight) edge->height = global.minBarHeight;
            
            edge->color = HSVToRGB(
                    (struct HSV){global.colorStart + edge->location * global.colorRange,
                    global.colorSaturation,
                    global.colorBrightness});
            
            currentBin = nextBinValue;
        }
        struct Edge *edge = &channelData->edge[0];
        edge->height = 0;
        edge->location = -1 * channelData->edge[2].location;
        edge->color = HSVToRGB(
                    (struct HSV){global.colorStart + edge->location * global.colorRange,
                    global.colorSaturation,
                    global.colorBrightness});
        
        edge = &channelData->edge[global.numBars + 1];
        edge->height = 0;
        edge->location = 2 - channelData->edge[global.numBars - 1].location;
        // edge->location = 1.3;
        edge->color = HSVToRGB(
                    (struct HSV){global.colorStart + edge->location * global.colorRange,
                    global.colorSaturation,
                    global.colorBrightness});
        
    }
}

void waveDataToLineVertexData(struct ChannelData *channelData, struct Vector2D startPoint, struct Vector2D endPoint, struct Vector2D barDirection)
{
    for (int64_t i = 0; i < global.numBars + 2; i++)
    {
        struct Edge *edge = &channelData->edge[i];
        struct Vertex *vertex0 = &channelData->vertex[i * 2 + 0];
        struct Vertex *vertex1 = &channelData->vertex[i * 2 + 1];
        struct Vector2D coordinate = lerp2d(startPoint, endPoint, edge->location);
        struct Vector2D barTip = addPortion2d(coordinate, barDirection, edge->height);
        vertex0->xyz.x = coordinate.x;
        vertex0->xyz.y = coordinate.y;
        vertex0->xyz.z = 0;
        vertex0->rgba.r = edge->color.r;
        vertex0->rgba.g = edge->color.g;
        vertex0->rgba.b = edge->color.b;
        vertex0->rgba.a = 1;
        vertex1->xyz.x = barTip.x;
        vertex1->xyz.y = barTip.y;
        vertex1->xyz.z = 0;
        vertex1->rgba.r = edge->color.r;
        vertex1->rgba.g = edge->color.g;
        vertex1->rgba.b = edge->color.b;
        vertex1->rgba.a = 1;
    }
}

// void glfwSpectrum(double *soundArray, int64_t numBars, double barWidth, int64_t numChannels, int64_t channel, bool upright, bool isCircle)
void glfwSpectrum(struct AllChannelData *allChannelData)
{
    double biggestNoiseScale = 0.05;
    double smallestTimeScale = 0.00000000008;
    double tilt = (OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 1, 0) * 1
        + OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 2, 10) * 0.5
        + OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 4, 20) * 0.25) * biggestNoiseScale;
    double bottom;
    static int64_t old_time = 0;
    int64_t new_time = nanoTime();
    int64_t delta_time = new_time - old_time;
    if (old_time == 0) delta_time = 0;
    old_time = new_time;
    
    global.colorStart += global.colorSpeed * delta_time / (double)1000000000;
    
    if (global.wave == true)
    {
        calculateWaveData(allChannelData);
    }
    
    
    
    for (int64_t channel = 0; channel < allChannelData->numberOfChannels; channel++)
    {
        if (global.wave == true)
        {
            if (global.barMode == BAR)
            {
                struct Vector2D startPoint, endPoint, barDirection;
                if (channel == 0)
                {
                    startPoint.x = 0 + offset.x;
                    startPoint.y = 0 + offset.y;
                    endPoint.x = 0 + offset.x;
                    endPoint.y = 1 + offset.y;
                    barDirection.x = 0.5 * sizeRatio;
                    barDirection.y = 0;
                    waveDataToLineVertexData(allChannelData->channelDataArray[channel], startPoint, endPoint, barDirection);
                }
                else if (channel == 1)
                {
                    startPoint.x = 1 + offset.x;
                    startPoint.y = 0 + offset.y;
                    endPoint.x = 1 + offset.x;
                    endPoint.y = 1 + offset.y;
                    barDirection.x = -0.5 * sizeRatio;
                    barDirection.y = 0;
                    waveDataToLineVertexData(allChannelData->channelDataArray[channel], startPoint, endPoint, barDirection);
                }
                else
                {
                    startPoint.x = 1;
                    startPoint.y = 0;
                    endPoint.x = 1;
                    endPoint.y = 1;
                    barDirection.x = -0.5;
                    barDirection.y = 0;
                    waveDataToLineVertexData(allChannelData->channelDataArray[channel], startPoint, endPoint, barDirection);
                }
                
            }
            else if (global.barMode == NEWWAVE)
            {
                struct Vector2D barDirection, centralDirection;
                if (channel == 0)
                {
                    centralDirection = substract2d(centralLine.end, centralLine.start);
                    barDirection.x = centralDirection.y * xRatio / yRatio * sizeRatio * -0.5;
                    barDirection.y = centralDirection.x * yRatio / xRatio * sizeRatio * 0.5;
                    waveDataToLineVertexData(allChannelData->channelDataArray[channel], add2d(centralLine.start, offset), add2d(centralLine.end, offset), barDirection);
                }
                else if (channel == 1)
                {
                    centralDirection = substract2d(centralLine.end, centralLine.start);
                    barDirection.x = centralDirection.y * xRatio / yRatio * sizeRatio * 0.5;
                    barDirection.y = centralDirection.x * yRatio / xRatio * sizeRatio * -0.5;
                    waveDataToLineVertexData(allChannelData->channelDataArray[channel], add2d(centralLine.start, offset), add2d(centralLine.end, offset), barDirection);
                }
                
            }
        }
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    for (int64_t channel = 0; channel < allChannelData->numberOfChannels; channel++)
    {
        glVertexPointer(2, GL_FLOAT, sizeof(struct Vertex), (GLvoid*)allChannelData->channelDataArray[channel]->vertex);
        glColorPointer(3, GL_FLOAT, sizeof(struct Vertex), (GLvoid*)allChannelData->channelDataArray[channel]->vertex + 3 * sizeof(GL_FLOAT));
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * (global.numBars + 2));
    }
    
}



static int swap_tear;
static int swap_interval;
static double frameRate;

static void update_window_title(GLFWwindow* window)
{
    char title[256];

    snprintf(title, sizeof(title), "Acid Analyzer %.2f fps, refresh rate %.2f",
             frameRate, global.fps);

    glfwSetWindowTitle(window, title);
}

static void set_swap_interval(GLFWwindow* window, int interval)
{
    swap_interval = interval;
    glfwSwapInterval(swap_interval);
    update_window_title(window);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

unsigned long frameCount = 0;
double lastTime, currentTime;
GLFWwindow* window;
GLuint vertex_buffer, vertex_shader, fragment_shader, program;
GLint mvp_location, vpos_location;

void killAll()
{
    glfwTerminate();
    // endwin();
    killNcurses();
    exit(EXIT_SUCCESS);
}

void windowCloseCallback(GLFWwindow* window)
{
    killAll();
}

void toggleFullscreen(GLFWwindow* window)
{
    static int xPos = 100, yPos = 100, width = 720, height = 720;
    if (glfwGetWindowMonitor(window) == NULL)
    {
        glfwGetWindowPos(window, &xPos, &yPos);
        glfwGetWindowSize(window, &width, &height);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        global.fps = mode->refreshRate;
    }
    else
    {
        glfwSetWindowMonitor(window, NULL, xPos, yPos, width, height, GLFW_DONT_CARE);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_Q)
        {
            windowCloseCallback(window);
        }
        if (key == GLFW_KEY_M)
        {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        if (key == GLFW_KEY_F)
        {
            toggleFullscreen(window);
        }
        if (key == GLFW_KEY_ESCAPE && (glfwGetWindowMonitor(window) != NULL))
        {
            toggleFullscreen(window);
        }
        if (key == GLFW_KEY_I)
        {
            global.colorStart += 0.5;
        }
        if (key == GLFW_KEY_Z)
        {
            offset.x = 0;
            offset.y = 0;
        }
    }
    if (action == GLFW_REPEAT || action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_UP)
        {
            sizeInt++;
        }
        if (key == GLFW_KEY_DOWN)
        {
            sizeInt--;
        }
        if (key == GLFW_KEY_RIGHT)
        {
            if (emptyCircleRatio < 1)
            {
                emptyCircleRatio += 0.01;
            }
            else
            {
                emptyCircleRatio *= 1.01;
            }
            xSizeInt++;
        }
        if (key == GLFW_KEY_LEFT)
        {
            if (emptyCircleRatio < 1)
            {
                emptyCircleRatio -= 0.01;
            }
            else
            {
                emptyCircleRatio *= 0.99;
            }
            xSizeInt--;
        }
    }
}


int cp_x;
int cp_y;
int offset_cpx;
int offset_cpy;
int w_posx;
int w_posy;
int buttonEvent;

void cursorPositionCallback(GLFWwindow* window, double x, double y){
    if (buttonEvent == 1) {
        offset_cpx = x - cp_x;
        offset_cpy = y - cp_y;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        buttonEvent = 1;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        cp_x = floor(x);
        cp_y = floor(y);
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        buttonEvent = 0;
        cp_x = 0;
        cp_y = 0;
    }
    
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        double x, y;
        int32_t width, height;
        glfwGetCursorPos(window, &x, &y);
        glfwGetFramebufferSize(window, &width, &height);
        centralLine.start.x = x / width;
        centralLine.start.y = (height - y) / height;
        offset.x = 0;
        offset.y = 0;
    }
    
}

double getRefreshRate()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    return mode->refreshRate;
}

void preInitializeWindow()
{
    /*
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    */
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    // glfwWindowHint(GLFW_SAMPLES, 16);
    // glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
}

void postInitializeWindow()
{
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    set_swap_interval(window, 1);
    // glEnable(GL_MULTISAMPLE);
    
    swap_tear = (glfwExtensionSupported("WGL_EXT_swap_control_tear") ||
                 glfwExtensionSupported("GLX_EXT_swap_control_tear"));
    printf("swap_tear = %d\n", swap_tear);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetWindowOpacity(window, global.colorOpacity);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f); // left, right, bottom, top, near, far
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
}

void initializeGlfw()
{

    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit())
        exit(EXIT_FAILURE);
    
    preInitializeWindow();
    
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // window = glfwCreateWindow(1920, 1080, "AcidAnalyzer", glfwGetPrimaryMonitor(), NULL);
    window = glfwCreateWindow(720, 720, "Acid Analyzer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    postInitializeWindow();
    
    lastTime = glfwGetTime();
    frameRate = 0.0;
    
    // OpenSimplex2F(nanoTime(), &simplexContext);
    OpenSimplex2F(time(NULL), &simplexContext);
}

void startFrame()
{
    int width, height;
    mat4x4 m, p, mvp;
    
    glfwGetFramebufferSize(window, &width, &height);
    
    if (height > width)
    {
        xRatio = 1;
        yRatio = (double)width / height;
    }
    else
    {
        xRatio = (double)height / width;
        yRatio = 1;
    }
    
    sizeRatio = pow(1.01, sizeInt);
    // xRatio *= sizeRatio;
    // yRatio *= sizeRatio;
    minRatio = fmin(xRatio, yRatio);
    
    glViewport(0, 0, width, height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glClear(GL_COLOR_BUFFER_BIT);
    
    currentNanoTime = nanoTime();
}

void finalizeFrame()
{
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    frameCount++;
    
    currentTime = glfwGetTime();
    static double lastMovementTime = 0;
    
    static struct Vector2D oldCursorPosition = {100000, 100000};
    struct Vector2D newCursorPosition;
    glfwGetCursorPos(window, &newCursorPosition.x, &newCursorPosition.y);
    if (newCursorPosition.x == oldCursorPosition.x && newCursorPosition.y == oldCursorPosition.y)
    {
        if (currentTime > lastMovementTime + 5)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }
    else
    {
        lastMovementTime = currentTime;
        oldCursorPosition = newCursorPosition;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
    {
        int width, height;
        bool windowIsDecorated = glfwGetWindowAttrib(window, GLFW_DECORATED);
        glfwGetWindowSize(window, &width, &height);
        glfwDestroyWindow(window);
        preInitializeWindow();
        if (windowIsDecorated)
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        }
        glfwCreateWindow(width, height, "Acid Analyzer", NULL, NULL);
        postInitializeWindow();
    }
    
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        int width, height;
        bool windowIsFloating = glfwGetWindowAttrib(window, GLFW_FLOATING);
        glfwGetWindowSize(window, &width, &height);
        glfwDestroyWindow(window);
        preInitializeWindow();
        if (windowIsFloating)
        {
            glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        }
        glfwCreateWindow(width, height, "Acid Analyzer", NULL, NULL);
        postInitializeWindow();
    }
    
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        int width, height;
        bool windowIsFloating = glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER);
        glfwGetWindowSize(window, &width, &height);
        glfwDestroyWindow(window);
        preInitializeWindow();
        if (windowIsFloating)
        {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        }
        glfwCreateWindow(width, height, "Acid Analyzer", NULL, NULL);
        postInitializeWindow();
    }
    
    if (currentTime - lastTime > 10.0)
    {
        frameRate = frameCount / (currentTime - lastTime);
        frameCount = 0;
        lastTime = currentTime;
        global.fps = getRefreshRate();
        update_window_title(window);
    }
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        glfwGetWindowPos(window, &w_posx, &w_posy);
        int64_t new_w_posx = w_posx + offset_cpx;
        int64_t new_w_posy = w_posy + offset_cpy;
        glfwSetWindowPos(window, new_w_posx, new_w_posy);
        int32_t actual_w_posx, actual_w_posy;
        glfwGetWindowPos(window, &actual_w_posx, &actual_w_posy);
        if (offset_cpx != 0) errorX = new_w_posx - actual_w_posx;
        if (offset_cpy != 0) errorY = actual_w_posy - new_w_posy;
        offset_cpx = 0;
        offset_cpy = 0;
        // cp_x += offset_cpx;
        // cp_y += offset_cpy;
        int32_t width, height;
        glfwGetWindowSize(window, &width, &height);
        offset.x = (double)errorX / width;
        offset.y = (double)errorY / height;
        
    }
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        double x, y;
        int32_t width, height;
        glfwGetCursorPos(window, &x, &y);
        glfwGetFramebufferSize(window, &width, &height);
        centralLine.end.x = x / width;
        centralLine.end.y = (height - y) / height;
    }

    
    startFrame();
    
    // glfwTerminate();
    // exit(EXIT_SUCCESS);
}

