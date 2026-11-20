"""Scripted dive profile shared by the sensor peers. This is a declared trajectory, not a vehicle plant: the firmware
under test is open loop (it only forwards Jetson commands to the thrusters), so nothing here is fed back."""
T_GO, T_LEVEL, T_TURN, T_UP, T_STOP = 12.0, 20.0, 22.0, 30.0, 36.0
DEPTH_MAX, SPEED, YAW_END = 1.5, 0.6, 90.0


def _ramp(t, t0, t1):
    return 0.0 if t <= t0 else 1.0 if t >= t1 else (t - t0) / (t1 - t0)


def depth(t):                      # m, positive down
    return DEPTH_MAX * (_ramp(t, T_GO, T_LEVEL) - _ramp(t, T_UP, T_STOP))


def vdown(t):                      # m/s, derivative of depth
    return DEPTH_MAX * (1 / (T_LEVEL - T_GO) * (T_GO < t < T_LEVEL) - 1 / (T_STOP - T_UP) * (T_UP < t < T_STOP))


def speed(t):                      # m/s forward
    return SPEED * (_ramp(t, T_GO, T_GO + 3) - _ramp(t, T_STOP - 3, T_STOP))


def yaw(t):                        # deg, 0..360
    return YAW_END * _ramp(t, T_TURN, T_UP)


def pos(t, dt=0.05):               # dead-reckoned x (forward, m), y
    import math
    x = y = 0.0
    k = 0.0
    while k < t:
        s, h = speed(k), math.radians(yaw(k))
        x += s * math.cos(h) * dt
        y += s * math.sin(h) * dt
        k += dt
    return x, y
