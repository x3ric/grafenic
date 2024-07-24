
GLint GLuint1f(Shader shader,float in,const char* var){
    glUniform1f(glGetUniformLocation(shader.Program, var), in);
}

GLint GLuint2f(Shader shader,float in1,float in2,const char* var){
    glUniform2f(glGetUniformLocation(shader.Program, var), in1, in2);
}

GLint GLumatrix4fv(Shader shader,GLfloat* in,const char* var){
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, var), 1, GL_FALSE, in);
}
