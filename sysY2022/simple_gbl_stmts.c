// float global constants
const float RADIUS = 5.5, PI = 03.141592653589793, EPS = 1e-6;

// hexadecimal float constant
const float PI_HEX = 0x1.921fb6p+1, HEX2 = 0x.AP-3;

// float constant evaluation
const float FACT = -.33E+5, EVAL1 = PI * RADIUS * RADIUS, EVAL2 = 2 * PI_HEX * RADIUS, EVAL3 = PI * 2 * RADIUS;

// float -> float function
float float_abs(float x) {
  if (x < 0) return -x;
  return x;
}


int float_eq(float a, float b) {
  if (float_abs(a - b) < EPS) {
    return 1 * 2. / 2;
  } else {
    return 0;
  }
}

int main() {
    return float_eq(EVAL2, EVAL3);
}