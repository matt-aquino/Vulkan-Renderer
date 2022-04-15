
// Mandelbrot function: f(Zn+1) = Zn^2 + c, where c is a complex number: a + bi 
// https://en.wikipedia.org/wiki/Mandelbrot_set
// as we iterate, we update Z to be c, so f(0) = 0, f(1) = c, f(2) = c^2 + c, etc
// so we use our pixel coord as our imaginary number c

#version 460
#define TWO_PI 2.0f * 3.14159265f

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants
{
    float width;        // screen width
    float height;       // screen height
};

layout(binding = 0) uniform UBO
{
    float xOffset;
    float yOffset;
    float zoom;
	int numIterations;  // max num iterations
	float threshold;    // divergence bounds (since we can't use infinity)
};

#define resolution vec2(width, height)
#define fragCoord vec2(gl_FragCoord.x, gl_FragCoord.y)

// ============== Complex Number Operations =========================
// Inigo Quilez uses these in his Julia set distance shader: https://www.shadertoy.com/view/4dXGDX
// i modified them slightly to make them more readable to me, and explain them if i can
// i'll keep the ones i don't use for reference later.

// add a float to the real half of a complex number
vec2 complexAdd(vec2 a, float s) 
{ 
    return vec2(a.x + s, a.y); 
}

// multiply two complex numbers together
// (a + bi) * (c + di) = ac + adi + bci + bdi^2
// i^2 == -1, so bdi^2 = -bd
// move the real parts together, factor out i in imaginary part ==> (ac - bd) + (ad + bc)i
vec2 complexMul(vec2 a, vec2 b) 
{ 
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// find the conjugate of a complex number
// according to Wikipedia: https://en.wikipedia.org/wiki/Complex_conjugate#:~:text=In%20mathematics%2C%20the%20complex%20conjugate,magnitude%20but%20opposite%20in%20sign.
// "In mathematics, the complex conjugate of a complex number is the number with an equal real part 
// and an imaginary part equal in magnitude but opposite in sign. That is, if a and b are real, then 
// the complex conjugate of a + bi is equal to a - bi." 
vec2 complexConj(vec2 z) 
{ 
    return vec2(z.x, -z.y); 
}

// divide two complex numbers. you need to multiply both the numerator and denominator by
// the conjugate of the complex number

// a + bi    (a + bi)(c - di)      ac - adi + bci - bidi     (ac + bd) + (bc - ad)i
// ------  = ----------------- =  ----------------------- =    -----------------
// c + di    (c + di)(c - di)         (c^2 - d^2i^2)              (c^2 + d^2)

// ac + bd is just the dot product of z1 and z2 
// bc - ad, we calculate as normal and multiply terms
// c^2 + d^2 is just the square of z2, which we can get by dot(z2, z2)
vec2 complexDiv(vec2 z1, vec2 z2) 
{ 
    float d = dot(z2, z2); 
    return vec2(dot(z1, z2), z1.y * z2.x - z1.x * z2.y) / d; 
}

// square a complex number
// (a + bi) * (a + bi) => a^2 + 2abi + b^2i^2 ==> a^2 + 2abi - b^2
// move the real parts together => a^2 - b^2 + 2abi
vec2 complexSqr(vec2 a) 
{ 
    return vec2(a.x * a.x - a.y * a.y, 2.0 * a.x * a.y); 
}

// find the square root of a complex number
// a little more complicated to explain, due to checking equality and signs,
// so refer to this website: https://www.cuemath.com/algebra/square-root-of-complex-number/
vec2 complexSqrt(vec2 z) 
{ 
    float m = length(z); 
    return sqrt(0.5 * vec2(m + z.x, m - z.x)) * vec2(1.0, sign(z.y)); 
}

// raise a complex number z by some power n
// also complicated to explain, so refer to the link
// https://brilliant.org/wiki/complex-exponentiation/#:~:text=Complex%20exponentiation%20extends%20the%20notion,y%20is%20a%20complex%20number.
vec2 complexPow(vec2 z, float n) 
{ 
    float r = length(z); 
    float a = atan(z.y, z.x); 
    return pow(r, n) * vec2(cos(a * n), sin(a * n)); 
}

//=================================================

float calcMandelbrot(vec2 pixel)
{
    pixel = (2.0 * pixel - resolution) / resolution.y;
    pixel.y *= -1.0; // flip the y coord

    // scale coords to lie in the Mandelbrot scale
    // xoffset/yoffset are user inputs to pan across the fractal
    vec2 c = pixel * pow(0.5, zoom) + vec2(xOffset, yOffset);
    vec2 z = vec2(0.0);
    float n = 0.0;
    for (int i = 0; i < numIterations; i++)
    {
        if (dot(z, z) > threshold) 
          break;
        
        z = complexSqr(z) + c;
        n++;
    }

    return n / float(numIterations);
}

void main()
{
    float t = calcMandelbrot(fragCoord);
    
    // use a cosine palette to determine color:
    // http://iquilezles.org/www/articles/palettes/palettes.htm
    vec3 j = vec3(0.5f);
    vec3 k = vec3(0.5f);
    vec3 l = vec3(1.0f);
    vec3 m = vec3(0.3f,0.2f,0.2f); 
    fragColor = vec4(j + k * cos(TWO_PI * (l * t + m)), 1.0f);
}
