

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

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#include <defaults.h>
extern struct Global global;

struct OpenSimplex2F_context *simplexContext;

static int64_t currentNanoTime = 0;

static double xRatio = 1;
static double yRatio = 1;

static double offsetX = 0.0;
static double offsetY = 0.0;
static int64_t errorX = 0;
static int64_t errorY = 0;

static int64_t sizeInt = 0;
static double emptyCircleRatio = 0.2;

float properSin(float ratioAngle)
{
    return sinf(ratioAngle * 2 * PI);
}

float properCos(float ratioAngle)
{
    return cosf(ratioAngle * 2 * PI);
}

double smoothStep(double x)
{
   return x * x * (3.0 - 2.0 * x);
}

double lerp(double a, double b, double f) 
{
    return (a * (1.0 - f)) + (b * f);
}

double smootherStep(double x)
{
   return x * x * x * (x * (6.0 * x - 15.0) + 10.0);
}

struct RGB
{
	float R;
	float G;
	float B;
};

struct HSV
{
	float H;
	float S;
	float V;
};

struct RGB HSVToRGB(struct HSV hsv)
{
	struct RGB rgb;

	if (hsv.S <= 0)
	{
		rgb.R = hsv.V;
		rgb.G = hsv.V;
		rgb.B = hsv.V;
	}
	else
	{
		int i;
		double f, p, q, t;
		if (hsv.S > 1.0) hsv.S = 1.0; 

		if (hsv.H <= 0)
			hsv.H = 0;
		else
		{
			hsv.H = fmodf(hsv.H, 1.0);
		}

		i = (int)trunc(hsv.H * 6.0);
		f = hsv.H * 6.0 - i;

		p = hsv.V * (1.0 - hsv.S);
		q = hsv.V * (1.0 - (hsv.S * f));
		t = hsv.V * (1.0 - (hsv.S * (1.0 - f)));

		switch (i)
		{
		case 0:
			rgb.R = hsv.V;
			rgb.G = t;
			rgb.B = p;
			break;

		case 1:
			rgb.R = q;
			rgb.G = hsv.V;
			rgb.B = p;
			break;

		case 2:
			rgb.R = p;
			rgb.G = hsv.V;
			rgb.B = t;
			break;

		case 3:
			rgb.R = p;
			rgb.G = q;
			rgb.B = hsv.V;
			break;

		case 4:
			rgb.R = t;
			rgb.G = p;
			rgb.B = hsv.V;
			break;

		default:
			rgb.R = hsv.V;
			rgb.G = p;
			rgb.B = q;
			break;
		}
	}
	return rgb;
}

void glfwSpectrum(double *soundArray, int64_t numBars, double barWidth, int64_t numChannels, int64_t channel, bool upright, bool isCircle)
{
    float biggestNoiseScale = 0.05;
    float smallestTimeScale = 0.00000000008;
    float tilt = (OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 1, 0) * 1
        + OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 2, 10) * 0.5
        + OpenSimplex2F_noise2(simplexContext, currentNanoTime * smallestTimeScale * 4, 20) * 0.25) * biggestNoiseScale;
    float bottom;
    if (!upright && !isCircle)
    {
        glColor3f(0.8f, 0.8f, 0.8f);
        bottom = ((channel + 1) / (float)numChannels) * 2 - 1;
    }
    else
    {
        glColor3f(1.0f, 1.0f, 1.0f);
        bottom = (channel / (float)numChannels) * 2 - 1;
    }
    
    for (int64_t barIndex = 0; barIndex < numBars; barIndex++)
    {
        float barHeigth = soundArray[barIndex];
        float leftBarEdgeLocation = (barIndex + 0.5 - barWidth * 0.5) / (float)numBars;
        float rightBarEdgeLocation = (barIndex + 0.5 + barWidth * 0.5) / (float)numBars;
        struct RGB leftRgb = HSVToRGB(
                (struct HSV){lerp(smoothStep(leftBarEdgeLocation), leftBarEdgeLocation, 0.6) * 0.666,
                1,
                1});
        struct RGB rightRgb = HSVToRGB(
                (struct HSV){lerp(smoothStep(rightBarEdgeLocation), rightBarEdgeLocation, 0.6) * 0.666,
                1,
                1});
        // glColor3f(rgb.R, rgb.G, rgb.B);
        if (barHeigth < 0) barHeigth = 0;
        
        float left = (barIndex + 0.5 - barWidth * 0.5) / (float)numBars * 2 - 1;
        float right = (barIndex + 0.5 + barWidth * 0.5) / (float)numBars * 2 - 1;
        float top;
        if (upright) top = bottom + barHeigth * 2 / numChannels;
        else top = bottom - barHeigth * 2 / numChannels;
        
        if (isCircle && numChannels < 3)
        {
            float leftCos, leftSin, rightCos, rightSin;
            if (channel == 0)
            {
                leftCos = properCos(0.75 - leftBarEdgeLocation / 2 + tilt) * xRatio;
                leftSin = properSin(0.75 - leftBarEdgeLocation / 2 + tilt) * yRatio;
                rightCos = properCos(0.75 - rightBarEdgeLocation / 2 + tilt) * xRatio;
                rightSin = properSin(0.75 - rightBarEdgeLocation / 2 + tilt) * yRatio;
            }
            else
            {
                leftCos = properCos(0.75 + leftBarEdgeLocation / 2 + tilt) * xRatio;
                leftSin = properSin(0.75 + leftBarEdgeLocation / 2 + tilt) * yRatio;
                rightCos = properCos(0.75 + rightBarEdgeLocation / 2 + tilt) * xRatio;
                rightSin = properSin(0.75 + rightBarEdgeLocation / 2 + tilt) * yRatio;
            }
            /*
            glBegin(GL_QUADS);
            glVertex2f(leftCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, leftSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY);
            glVertex2f(rightCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, rightSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY);
            // glVertex2f(right, bottom);
            // glVertex2f(left, bottom);
            glVertex2f(rightCos * emptyCircleRatio + offsetX, rightSin * emptyCircleRatio + offsetY);
            glVertex2f(leftCos * emptyCircleRatio + offsetX, leftSin * emptyCircleRatio + offsetY);
            glEnd();
            */
            
            if (global.openglVersion < 3)
            {
                glBegin(GL_QUADS);
                glColor4f(rightRgb.R, rightRgb.G, rightRgb.B, 0);
                glVertex2f(rightCos * emptyCircleRatio + offsetX, rightSin * emptyCircleRatio + offsetY);
                glColor4f(leftRgb.R, leftRgb.G, leftRgb.B, 0);
                glVertex2f(leftCos * emptyCircleRatio + offsetX, leftSin * emptyCircleRatio + offsetY);
                // glColor4f(1,0,0, 0.5);
                glColor4f(leftRgb.R, leftRgb.G, leftRgb.B, 1);
                glVertex2f(leftCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, leftSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY);
                glColor4f(rightRgb.R, rightRgb.G, rightRgb.B, 1);
                glVertex2f(rightCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, rightSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY);
                glEnd();
            }
            else
            {
                float vertices[4 * 2] =
                {
                    leftCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, leftSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY,
                    rightCos * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetX, rightSin * ((1 - emptyCircleRatio) * barHeigth + emptyCircleRatio) + offsetY,
                    leftCos * emptyCircleRatio + offsetX, leftSin * emptyCircleRatio + offsetY,
                    rightCos * emptyCircleRatio + offsetX, rightSin * emptyCircleRatio + offsetY
                };
                GLfloat colors[] =
                {
                    leftRgb.R, leftRgb.G, leftRgb.B,
                    rightRgb.R, rightRgb.G, rightRgb.B,
                    leftRgb.R, leftRgb.G, leftRgb.B,
                    rightRgb.R, rightRgb.G, rightRgb.B
                };
            
                GLubyte indices[] = {0,1,2, // first triangle (bottom left - top left - top right)
                         1,2,3}; // second triangle (bottom left - top right - bottom right)
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
                
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_COLOR_ARRAY);
                
                glVertexPointer(2, GL_FLOAT, 0, vertices);
                glColorPointer(3, GL_FLOAT, 0, colors);
            }
        }
        else
        {
            glBegin(GL_QUADS);
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glEnd();
        }
    }
}

static int swap_tear;
static int swap_interval;
static double frameRate;

static void update_window_title(GLFWwindow* window)
{
    char title[256];

    snprintf(title, sizeof(title), "Acid Analyzer %0.01f fps",
             frameRate);

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
    }
    else
    {
        glfwSetWindowMonitor(window, NULL, xPos, yPos, width, height, GLFW_DONT_CARE);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
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

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        buttonEvent = 1;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        cp_x = floor(x);
        cp_y = floor(y);
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
        buttonEvent = 0;
        cp_x = 0;
        cp_y = 0;
    }
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
    
    
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glfwSetWindowOpacity(window, 0.5f);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f); // left, right, bottom, top, near, far
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
    
    double sizeRatio = pow(1.01, sizeInt);
    xRatio *= sizeRatio;
    yRatio *= sizeRatio;
    
    offsetX = (errorX * 2.0) / width;
    offsetY = (errorY * 2.0) / height;
    
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
    
    currentTime = glfwGetTime();
    if (currentTime - lastTime > 10.0)
    {
        frameRate = frameCount / (currentTime - lastTime);
        frameCount = 0;
        lastTime = currentTime;
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
    }
    
    // glfwTerminate();
    // exit(EXIT_SUCCESS);
}

