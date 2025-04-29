import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.patches import Polygon
from matplotlib.path import Path

# --- Parameters (all lengths in mm) ---
cart_length = 65     # length of each cart
cart_width = 65      # width of each cart
gap = 25            # gap from the rear of one cart to the front of the next

# The coupler (front attachment point) is assumed to be at the front of each cart.
# The separation between couplers is: cart_length + gap.
offset = cart_length + gap  # e.g. 70 + 20 = 90 mm

# For a 2‑cart system, define two coupler offsets:
# The lead cart is at 0 and the trailing cart is at –offset (behind the lead).
coupler_offsets = [0, -offset]

# --- Tube Parameters ---
tube_width = 85
half_tube = tube_width / 2  # 42.5 mm

# --- Path Offset Parameter ---
# This shifts the path the carts follow relative to the tube centerline.
path_offset = 8  # positive shifts to the left

# --- Nose Cone Parameter ---
# The nose cone (triangle) attached to the first cart will have the same length as the cart.
nose_length = cart_length

# --- Define the Original Path (Tube Centerline) ---
# The path is parameterized by arc-length s and is composed of three segments.
L1 = 300.0         # horizontal segment length (mm)
R = 100.0          # radius for the circular arc (mm)
L2 = (np.pi / 2) * R  # arc length for a 90° turn (mm)
L3 = 300.0         # vertical segment length (mm)
s_end = L1 + L2 + L3  # total arc-length for the front coupler

def P(s):
    """
    Given an arc-length parameter s (in mm), return (x, y, theta),
    where (x, y) is the position and theta (in radians) is the tangent (heading)
    of the tube centerline.
    """
    if s < 0:
        x = -300 + s
        y = 42.5
        theta = 0.0
    elif s < L1:
        x = -300 + s
        y = 42.5
        theta = 0.0
    elif s < L1 + L2:
        theta_arc = (s - L1) / R
        x = R * np.sin(theta_arc)
        y = (42.5 + R) - R * np.cos(theta_arc)
        theta = theta_arc
    else:
        s2 = s - (L1 + L2)
        x = R
        y = (42.5 + R) + s2
        theta = np.pi / 2
    return x, y, theta

def unit_normal(theta):
    """Return the unit vector perpendicular to (cos theta, sin theta) pointing to the left."""
    return np.array([-np.sin(theta), np.cos(theta)])

def P_offset(s, lateral_offset=0):
    """
    Returns the point on the path shifted laterally by 'lateral_offset' (in mm).
    (Positive lateral_offset shifts to the left.)
    
    Returns: (x_offset, y_offset, theta)
    """
    x, y, theta = P(s)
    n = unit_normal(theta)
    return x + lateral_offset * n[0], y + lateral_offset * n[1], theta

def cart_polygon(s_coupler):
    """
    Given the arc-length position (s_coupler) of a cart's coupler (front center)
    along the offset path, return a 4x2 numpy array with the (x, y) coordinates
    of the cart's rectangular corners, ordered as:
    [front_left, front_right, rear_right, rear_left].
    """
    # Get the front (coupler) point on the offset path.
    front = np.array(P_offset(s_coupler, path_offset)[0:2])
    _, _, theta = P_offset(s_coupler, path_offset)
    # The rear edge is cart_length behind the front.
    rear = front - cart_length * np.array([np.cos(theta), np.sin(theta)])
    # Compute the side offsets (half the cart's width).
    n = unit_normal(theta)
    offset_side = (cart_width / 2) * n
    front_left = front + offset_side
    front_right = front - offset_side
    rear_left = rear + offset_side
    rear_right = rear - offset_side
    return np.array([front_left, front_right, rear_right, rear_left])

def nose_cone(s_coupler):
    """
    Returns the vertices of a triangular nose cone attached flush to the front of the cart.
    The base of the triangle coincides with the front edge of the cart (from front_left to front_right),
    and the apex is located nose_length mm in front in the direction of travel.
    """
    # Get the rectangular cart polygon to obtain its front edge.
    poly = cart_polygon(s_coupler)
    front_left = poly[0]
    front_right = poly[1]
    front_center = (front_left + front_right) / 2
    theta = P_offset(s_coupler, path_offset)[2]
    apex = front_center + nose_length * np.array([np.cos(theta), np.sin(theta)])
    return np.array([front_left, front_right, apex])

def cart_center(s_coupler):
    """
    Computes the geometric center of a cart (the midpoint between its front and rear edges)
    given the coupler's arc-length position along the offset path.
    """
    front = np.array(P_offset(s_coupler, path_offset)[0:2])
    theta = P_offset(s_coupler, path_offset)[2]
    return front - 0.5 * cart_length * np.array([np.cos(theta), np.sin(theta)])

# --- Build the Tube Polygon for Plotting and Collision Detection ---
s_vals = np.linspace(-500, s_end + 100, 1000)
tube_left = []
tube_right = []
for s in s_vals:
    x, y, theta = P(s)
    center = np.array([x, y])
    n = unit_normal(theta)
    tube_left.append(center + half_tube * n)
    tube_right.append(center - half_tube * n)
tube_left = np.array(tube_left)
tube_right = np.array(tube_right)
# Create a closed polygon by traversing the left boundary and then the right boundary in reverse.
tube_polygon_pts = np.vstack((tube_left, tube_right[::-1]))
tube_path = Path(tube_polygon_pts)

# --- Set up the Plot ---
fig, ax = plt.subplots(figsize=(8, 8))
ax.set_aspect('equal')
ax.set_xlabel('x (mm)')
ax.set_ylabel('y (mm)')
ax.set_title('2-Cart System with Nose Cone on the First Cart')

# Plot the tube (filled with light gray)
tube_patch = Polygon(tube_polygon_pts, closed=True, facecolor='lightgray', edgecolor='gray', alpha=0.5)
ax.add_patch(tube_patch)

# Optionally, plot the tube centerline and the cart path (offset)
path_points = np.array([P(s)[0:2] for s in s_vals])
ax.plot(path_points[:, 0], path_points[:, 1], 'k--', lw=1, label='Tube Centerline')
offset_path_points = np.array([P_offset(s, path_offset)[0:2] for s in s_vals])
ax.plot(offset_path_points[:, 0], offset_path_points[:, 1], 'r--', lw=1, label='Cart Path')

# Create patches for the two carts.
# Both are drawn as rectangles.
cart_patches = []
for i in range(2):
    poly = Polygon(cart_polygon(0), closed=True, fc='blue', ec='k', lw=1)
    cart_patches.append(poly)
    ax.add_patch(poly)

# Create an extra patch for the nose cone attached to the first cart.
nose_patch = Polygon(nose_cone(0), closed=True, fc='blue', ec='k', lw=1)
ax.add_patch(nose_patch)

ax.set_xlim(-600, 200)
ax.set_ylim(-100, 600)
ax.legend()

# --- Animation Parameters ---
v = 50.0  # speed in mm/s
t_end = s_end / v + 2  # extra time so that the full train appears

def animate(frame):
    t = frame / 30.0  # assume 30 frames per second
    s_front = v * t   # arc-length position of the lead cart's coupler
    collision_found = False

    # Update each cart's rectangle using its coupler offset.
    for i, off in enumerate(coupler_offsets):
        s_cart = s_front + off
        poly_pts = cart_polygon(s_cart)
        cart_patches[i].set_xy(poly_pts)
        # Check collision: if any vertex is outside the tube, color it red.
        collides = any(not tube_path.contains_point(pt) for pt in poly_pts)
        if collides:
            cart_patches[i].set_facecolor('red')
            collision_found = True
        else:
            cart_patches[i].set_facecolor('blue')

    # Update the nose cone so that it stays flush with the front edge of the first cart.
    s_first = s_front + coupler_offsets[0]
    nose_patch.set_xy(nose_cone(s_first))
    
    if collision_found:
        ax.set_title(f'Collision detected at t={t:.1f} s', color='red')
    else:
        ax.set_title(f'2-Cart System with Nose Cone, t={t:.1f} s')
    return cart_patches + [nose_patch]

frames = int(t_end * 30)
ani = animation.FuncAnimation(fig, animate, frames=frames, interval=33, blit=False)
plt.show()
