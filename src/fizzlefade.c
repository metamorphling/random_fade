#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASK_BITS_SIZE 20
#define NIBBLE 4

void framebuffer_size_callback (GLFWwindow* window, int width, int height);
void process_input (GLFWwindow *window);

const char *vertex_shader_source = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
const char *fragment_shader_source = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n\0";

int find_power (int needed_resolution) {
    int power = 0, current_max_resolution = 1;
    while (current_max_resolution < needed_resolution) {
        current_max_resolution *= 2;
        power++;
    }
    return power;
}

int main (int argc, char *argv[]) {
    if (argc != 3) {
        printf ("Need 2 arguments: X and Y resolution of display\n");
        exit (0);
    }

    int res_x, res_y, result;
    result = sscanf (argv[1], "%d", &res_x);
    if (!result) {
        printf ("Failed to sscanf! \n");
        exit (0);
    }
    result = sscanf (argv[2], "%d", &res_y);
    if (!result) {
        printf ("Failed to sscanf! \n");
        exit (0);
    }
    if (res_x < 0 || res_x > 7680 || res_y < 0 || res_y > 7680) {
        printf ("Limiting to UHD resolution, set values from 0 to 7680\n");
        exit (0);
    }
    float half_resolution_x = res_x / 2, half_resolution_y = res_y / 2;

    FILE *fd = fopen ("coordinates", "w");
    if (fd == NULL) {
        printf ("Can't open file! \n");
        exit (0);
    }
    uint32_t random_value = 1, x_bits, y_bits, x_mask = 0xFFFFF, y_mask = 0xFFFFF;
    uint16_t x, y;
    x_bits = find_power (res_x); /* Power enough to satisfy resolution */
    y_bits = find_power (res_y);
    x_mask >>= (MASK_BITS_SIZE - x_bits); /* Mask on higher bits */
    x_mask <<= (x_bits / NIBBLE) * NIBBLE;
    y_mask >>= (MASK_BITS_SIZE - y_bits); /* Mask on lower bits */
    long unsigned int i = 0;
    do {
        y = random_value & y_mask; /* Y = low bits */
        x = (random_value & x_mask) >> 8; /* X = High bits */
        unsigned least_significant_bit = random_value & 1; /* Get the output bit. */
        random_value >>= 1; /* Shift register */
        if (least_significant_bit) { /* If the output is 0, the xor can be skipped. */
            random_value ^= 0x00012000;
        }
        if (x < res_x && y < res_y) {
            printf ("i:%lu x: %d y: %d \n", i++, x, y);
            fprintf (fd, "%f %f\n", (x - half_resolution_x) / half_resolution_x, (y - half_resolution_y) / half_resolution_y);
        }
    }
    while (random_value != 1);
    fclose (fd);

// glfw: initialize and configure
    glfwInit ();
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow (res_x, res_y, "Random Fade", NULL, NULL);

    if (window == NULL) {
        printf ("Failed to create GLFW window");
        glfwTerminate ();
        return -1;
    }
    glfwMakeContextCurrent (window);
    glfwSetFramebufferSizeCallback (window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader ((GLADloadproc) glfwGetProcAddress)) {
        printf ("Failed to initialize GLAD");
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    int vertex_shader = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader (vertex_shader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv (vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog (vertex_shader, 512, NULL, infoLog);
        printf ("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s \n", infoLog);
    }
    // fragment shader
    int fragment_shader = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader (fragment_shader);
    // check for shader compile errors
    glGetShaderiv (fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog (fragment_shader, 512, NULL, infoLog);
        printf ("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED %s\n", infoLog);
    }
    // link shaders
    int shader_program = glCreateProgram ();
    glAttachShader (shader_program, vertex_shader);
    glAttachShader (shader_program, fragment_shader);
    glLinkProgram (shader_program);
    // check for linking errors
    glGetProgramiv (shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog (shader_program, 512, NULL, infoLog);
        printf ("ERROR::SHADER::PROGRAM::LINKING_FAILED %s\n", infoLog);
    }
    glDeleteShader (vertex_shader);
    glDeleteShader (fragment_shader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    float *vertices;
    vertices = malloc (res_x * res_y);
    if (vertices == NULL) {
        printf ("Failed to allocate memory! \n");
        exit (0);
    }
    vertices[0] = 0.5f;
    vertices[1] = 0.5f;
    vertices[2] = 0.0f;
    // OpenGL vertex setup
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays (1, &VAO);
    glGenBuffers (1, &VBO);
    glGenBuffers (1, &EBO);
    glBindVertexArray (VAO);
    glBindBuffer (GL_ARRAY_BUFFER, VBO);
    glBufferData (GL_ARRAY_BUFFER, res_x * res_y, vertices, GL_STATIC_DRAW);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);

    unsigned long int k = 3;
    fd = fopen ("coordinates", "r");
    if (fd == NULL) {
        printf ("Can't open file! \n");
        exit (0);
    }

    // render loop
    while (!glfwWindowShouldClose (window)) {
        char * line = NULL;
        float x = 0, y = 0;
        size_t len;
        if (getline (&line, &len, fd) == -1)
            continue;
        if (sscanf (line, "%f %f", &x, &y) < 1)
            continue;
        free (line);
        process_input (window);
        glClearColor (0.2f, 0.3f, 0.3f, 1.0f);
        glClear (GL_COLOR_BUFFER_BIT);

        glUseProgram (shader_program);
        glBindVertexArray (VAO);
        glDrawArrays (GL_POINTS, 0, k / 3);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers (window);
        glfwPollEvents ();
        glBindBuffer (GL_ARRAY_BUFFER, VBO);
        // get pointer
        void *ptr = glMapBuffer (GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        // now copy data into memory
        vertices[k] = x;
        vertices[k + 1] = y;
        vertices[k + 2] = 0;
        k += 3;
        memcpy (ptr, vertices, k * 4);
        // make sure to tell OpenGL we're done with the pointer
        glUnmapBuffer (GL_ARRAY_BUFFER);
    }
    // closing, freeing, deallocating
    fclose (fd);
    free (vertices);
    glDeleteVertexArrays (1, &VAO);
    glDeleteBuffers (1, &VBO);
    glDeleteBuffers (1, &EBO);
    glfwTerminate ();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void process_input (GLFWwindow *window) {
    if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose (window, 1);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport (0, 0, width, height);
}
