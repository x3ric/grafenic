
void MatrixLookAt(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat* matrix) {
    GLfloat f[3] = { centerX - eyeX, centerY - eyeY, centerZ - eyeZ };
    GLfloat norm = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    f[0] /= norm; f[1] /= norm; f[2] /= norm;
    GLfloat s[3] = { f[1] * upZ - f[2] * upY, f[2] * upX - f[0] * upZ, f[0] * upY - f[1] * upX };
    norm = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] /= norm; s[1] /= norm; s[2] /= norm;
    GLfloat u[3] = { s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0] };
    matrix[0] = s[0]; matrix[1] = u[0]; matrix[2] = -f[0]; matrix[3] = 0.0f;
    matrix[4] = s[1]; matrix[5] = u[1]; matrix[6] = -f[1]; matrix[7] = 0.0f;
    matrix[8] = s[2]; matrix[9] = u[2]; matrix[10] = -f[2]; matrix[11] = 0.0f;
    matrix[12] = -s[0] * eyeX - s[1] * eyeY - s[2] * eyeZ;
    matrix[13] = -u[0] * eyeX - u[1] * eyeY - u[2] * eyeZ;
    matrix[14] = f[0] * eyeX + f[1] * eyeY + f[2] * eyeZ;
    matrix[15] = 1.0f;
}

void MatrixRotate(GLfloat angleX, GLfloat angleY, GLfloat angleZ, GLfloat* matrix) {
    GLfloat cx = cosf(angleX), sx = sinf(angleX);
    GLfloat cy = cosf(angleY), sy = sinf(angleY);
    GLfloat cz = cosf(angleZ), sz = sinf(angleZ);
    matrix[0] = cy * cz; matrix[1] = -cy * sz; matrix[2] = sy; matrix[3] = 0.0f;
    matrix[4] = sx * sy * cz + cx * sz; matrix[5] = -sx * sy * sz + cx * cz; matrix[6] = -sx * cy; matrix[7] = 0.0f;
    matrix[8] = -cx * sy * cz + sx * sz; matrix[9] = cx * sy * sz + sx * cz; matrix[10] = cx * cy; matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

void MatrixPerspective(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far, GLfloat* matrix) {
    GLfloat tanHalfFov = tanf(fov / 2.0f * M_PI / 180.0f);
    memset(matrix, 0, sizeof(GLfloat) * 16);
    matrix[0] = -1.0f / (aspect * tanHalfFov); // Negated "Flipped x axis" to be orthographic consistent
    matrix[5] = 1.0f / tanHalfFov;
    matrix[10] = -(far + near) / (far - near);
    matrix[11] = -1.0f;
    matrix[14] = -2.0f * far * near / (far - near);
    matrix[15] = 0.0f;
}

void MatrixOrthographic(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar, GLfloat *matrix) {
    if(camera.position.z <= 0.0f)
        camera.position.z += 1.0f;
    memset(matrix, 0, sizeof(GLfloat) * 16);
    matrix[0] = 2.0f * camera.position.z / SCREEN_WIDTH;       // Horizontal scaling factor
    matrix[5] = -2.0f * camera.position.z / SCREEN_HEIGHT;     // Vertical scaling factor
    matrix[10] = -2.0f / (zFar - zNear);                       // Depth scaling factor
    matrix[12] = -(right + left) / SCREEN_WIDTH * camera.position.z + camera.position.x * 2.0f * camera.position.z / SCREEN_WIDTH;  // X offset
    matrix[13] = (top + bottom) / SCREEN_HEIGHT * camera.position.z - camera.position.y * 2.0f * camera.position.z / SCREEN_HEIGHT; // Y offset
    matrix[14] = -(zFar + zNear) / (zFar - zNear);             // Maps the z-range to [-1, 1]
    matrix[15] = 1.0f;                                         // Cordinate Scale
}

void MatrixMultiply(const GLfloat* a, const GLfloat* b, GLfloat* result) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] +
                                a[i * 4 + 1] * b[1 * 4 + j] +
                                a[i * 4 + 2] * b[2 * 4 + j] +
                                a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
}

void MatrixIdentity(GLfloat* out) {
    memset(out, 0, 16 * sizeof(GLfloat));
    out[0] = out[5] = out[10] = out[15] = 1.0f;
}

void RotateVertex(GLfloat *x, GLfloat *y, GLfloat *z, GLfloat angleX, GLfloat angleY, GLfloat angleZ) {
    // Convert angles from degrees to radians
    GLfloat radX = angleX * (M_PI / 180.0f);
    GLfloat radY = angleY * (M_PI / 180.0f);
    GLfloat radZ = angleZ * (M_PI / 180.0f);
    // Rotation matrices for each axis
    GLfloat Rx[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cos(radX), -sin(radX), 0.0f,
        0.0f, sin(radX), cos(radX), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    GLfloat Ry[16] = {
        cos(radY), 0.0f, sin(radY), 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -sin(radY), 0.0f, cos(radY), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    GLfloat Rz[16] = {
        cos(radZ), -sin(radZ), 0.0f, 0.0f,
        sin(radZ), cos(radZ), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    // Combine rotations: R = Rz * (Ry * Rx)
    GLfloat Rxy[16];
    GLfloat R[16];
    // Multiply Ry by Rx
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            Rxy[i * 4 + j] = Ry[i * 4 + 0] * Rx[0 * 4 + j] +
                             Ry[i * 4 + 1] * Rx[1 * 4 + j] +
                             Ry[i * 4 + 2] * Rx[2 * 4 + j] +
                             Ry[i * 4 + 3] * Rx[3 * 4 + j];
        }
    }
    // Multiply Rz by Rxy
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            R[i * 4 + j] = Rz[i * 4 + 0] * Rxy[0 * 4 + j] +
                           Rz[i * 4 + 1] * Rxy[1 * 4 + j] +
                           Rz[i * 4 + 2] * Rxy[2 * 4 + j] +
                           Rz[i * 4 + 3] * Rxy[3 * 4 + j];
        }
    }
    // Apply combined rotation matrix to vertex
    GLfloat newX = R[0] * (*x) + R[1] * (*y) + R[2] * (*z) + R[3];
    GLfloat newY = R[4] * (*x) + R[5] * (*y) + R[6] * (*z) + R[7];
    GLfloat newZ = R[8] * (*x) + R[9] * (*y) + R[10] * (*z) + R[11];
    *x = newX;
    *y = newY;
    *z = newZ;
}
