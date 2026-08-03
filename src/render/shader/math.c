
void MatrixIdentity(float* out) {
    _mm_storeu_ps(out, _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f));
    _mm_storeu_ps(out + 4, _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    _mm_storeu_ps(out + 8, _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_storeu_ps(out + 12, _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));
}

void MatrixMultiply(const GLfloat* a, const GLfloat* b, GLfloat* result) {
    __m128 b0 = _mm_loadu_ps(b);
    __m128 b1 = _mm_loadu_ps(b + 4);
    __m128 b2 = _mm_loadu_ps(b + 8);
    __m128 b3 = _mm_loadu_ps(b + 12);
    GLfloat temp[16];
    for (int i = 0; i < 4; i++) {
        const GLfloat* row = a + i * 4;
        __m128 value = _mm_mul_ps(_mm_set1_ps(row[0]), b0);
        value = _mm_add_ps(value, _mm_mul_ps(_mm_set1_ps(row[1]), b1));
        value = _mm_add_ps(value, _mm_mul_ps(_mm_set1_ps(row[2]), b2));
        value = _mm_add_ps(value, _mm_mul_ps(_mm_set1_ps(row[3]), b3));
        _mm_storeu_ps(temp + i * 4, value);
    }
    memcpy(result, temp, sizeof(temp));
}

void MatrixLookAt(GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat* matrix) {
    GLfloat fx = centerX - eyeX;
    GLfloat fy = centerY - eyeY;
    GLfloat fz = centerZ - eyeZ;
    GLfloat norm = sqrtf(fx * fx + fy * fy + fz * fz);
    if (norm <= 0.000001f) {
        MatrixIdentity(matrix);
        return;
    }
    fx /= norm;
    fy /= norm;
    fz /= norm;
    GLfloat sx = fy * upZ - fz * upY;
    GLfloat sy = fz * upX - fx * upZ;
    GLfloat sz = fx * upY - fy * upX;
    norm = sqrtf(sx * sx + sy * sy + sz * sz);
    if (norm <= 0.000001f) {
        MatrixIdentity(matrix);
        return;
    }
    sx /= norm;
    sy /= norm;
    sz /= norm;
    GLfloat ux = sy * fz - sz * fy;
    GLfloat uy = sz * fx - sx * fz;
    GLfloat uz = sx * fy - sy * fx;
    matrix[0] = sx; matrix[1] = ux; matrix[2] = -fx; matrix[3] = 0.0f;
    matrix[4] = sy; matrix[5] = uy; matrix[6] = -fy; matrix[7] = 0.0f;
    matrix[8] = sz; matrix[9] = uz; matrix[10] = -fz; matrix[11] = 0.0f;
    matrix[12] = -sx * eyeX - sy * eyeY - sz * eyeZ;
    matrix[13] = -ux * eyeX - uy * eyeY - uz * eyeZ;
    matrix[14] = fx * eyeX + fy * eyeY + fz * eyeZ;
    matrix[15] = 1.0f;
}

void MatrixRotate(GLfloat angleX, GLfloat angleY, GLfloat angleZ, GLfloat* matrix) {
    GLfloat cx = cosf(angleX), sx = sinf(angleX);
    GLfloat cy = cosf(angleY), sy = sinf(angleY);
    GLfloat cz = cosf(angleZ), sz = sinf(angleZ);
    GLfloat rx[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cx, -sx, 0.0f,
        0.0f, sx, cx, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    GLfloat ry[16] = {
        cy, 0.0f, sy, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -sy, 0.0f, cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    GLfloat rz[16] = {
        cz, -sz, 0.0f, 0.0f,
        sz, cz, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    GLfloat temp[16];
    MatrixMultiply(ry, rx, temp);
    MatrixMultiply(rz, temp, matrix);
}

void MatrixTranslate(GLfloat tx, GLfloat ty, GLfloat tz, GLfloat* result) {
    MatrixIdentity(result);
    result[12] = tx;
    result[13] = ty;
    result[14] = tz;
}

void MatrixPerspective(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far, bool is3d, GLfloat* matrix) {
    if (aspect == 0.0f) aspect = 1.0f;
    if (near <= 0.0f) near = 0.01f;
    if (far <= near) far = near + 1000.0f;
    GLfloat tanHalfFov = tanf(fov * (PI / 360.0f));
    memset(matrix, 0, sizeof(GLfloat) * 16);
    matrix[0] = (is3d ? 1.0f : -1.0f) / (aspect * tanHalfFov);
    matrix[5] = 1.0f / tanHalfFov;
    matrix[10] = -(far + near) / (far - near);
    matrix[11] = -1.0f;
    matrix[14] = -2.0f * far * near / (far - near);
}

void MatrixOrthographic(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar, GLfloat* matrix) {
    GLfloat width = right - left;
    GLfloat height = top - bottom;
    GLfloat depth = zFar - zNear;
    if (width == 0.0f) width = 1.0f;
    if (height == 0.0f) height = 1.0f;
    if (depth == 0.0f) depth = 1.0f;
    memset(matrix, 0, 16 * sizeof(GLfloat));
    matrix[0] = 2.0f / width;
    matrix[5] = 2.0f / height;
    matrix[10] = -2.0f / depth;
    matrix[12] = -(right + left) / width;
    matrix[13] = -(top + bottom) / height;
    matrix[14] = -(zFar + zNear) / depth;
    matrix[15] = 1.0f;
}

void MatrixOrthographicZoom(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar, GLfloat zoomFactor, bool is3d, GLfloat* matrix) {
    if (!is3d) {
        if (zoomFactor <= 0.0f) zoomFactor = 1.0f;
        GLfloat width = right - left;
        GLfloat height = top - bottom;
        GLfloat zoomWidth = width / zoomFactor;
        GLfloat zoomHeight = height / zoomFactor;
        GLfloat adjustedLeft = left + (width - zoomWidth) * 0.5f;
        GLfloat adjustedRight = right - (width - zoomWidth) * 0.5f;
        GLfloat adjustedBottom = bottom + (height - zoomHeight) * 0.5f;
        GLfloat adjustedTop = top - (height - zoomHeight) * 0.5f;
        if (zNear <= 0.0f) zNear = 0.1f;
        if (zFar <= zNear) zFar = zNear + 0.1f;
        MatrixOrthographic(adjustedLeft, adjustedRight, adjustedBottom, adjustedTop, zNear, zFar, matrix);
        return;
    }
    if (zoomFactor >= 1.0f) zoomFactor = 1.0f;
    GLfloat aspectRatio = (GLfloat)window.screen_width / (GLfloat)window.screen_height;
    GLfloat orthoSize = 1.0f - zoomFactor;
    GLfloat left1 = orthoSize * aspectRatio;
    GLfloat right1 = -orthoSize * aspectRatio;
    GLfloat bottom1 = -orthoSize;
    GLfloat top1 = orthoSize;
    MatrixOrthographic(left1, right1, bottom1, top1, zNear, zFar, matrix);
    matrix[0] = -2.0f / (right1 - left1);
}

Vec3 MatrixMultiplyVector(const GLfloat matrix[16], Vec3 vector) {
    Vec3 result;
    result.x = matrix[0] * vector.x + matrix[4] * vector.y + matrix[8] * vector.z + matrix[12];
    result.y = matrix[1] * vector.x + matrix[5] * vector.y + matrix[9] * vector.z + matrix[13];
    result.z = matrix[2] * vector.x + matrix[6] * vector.y + matrix[10] * vector.z + matrix[14];
    GLfloat w = matrix[3] * vector.x + matrix[7] * vector.y + matrix[11] * vector.z + matrix[15];
    if (w != 1.0f && w != 0.0f) {
        GLfloat invW = 1.0f / w;
        result.x *= invW;
        result.y *= invW;
        result.z *= invW;
    }
    return result;
}

void TransformVertices(GLfloat* vertices, size_t vertexCount, const GLfloat* rotationMatrix, const Vec3* positionOffset) {
    for (size_t i = 0; i < vertexCount; i++, vertices += FLOAT_PER_VERTEX) {
        Vec3 vertex = {vertices[0], vertices[1], vertices[2]};
        Vec3 rotatedVertex = MatrixMultiplyVector(rotationMatrix, vertex);
        vertices[0] = rotatedVertex.x + positionOffset->x;
        vertices[1] = rotatedVertex.y + positionOffset->y;
        vertices[2] = rotatedVertex.z + positionOffset->z;
    }
}

Vec3 Vec3Add(const Vec3 vec1, const Vec3 vec2) {
    return (Vec3){vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z};
}

void CombineTransformation(GLfloat* modelMatrix, const GLfloat* translationMatrix, const GLfloat* rotationMatrix) {
    MatrixMultiply(translationMatrix, rotationMatrix, modelMatrix);
}
