"""
Blender headless renderer — PREMIUM 2-tone knob (UAD Pultec EQP-1A style).

Body: brushed aluminum cylinder with concentric machined grooves.
Cap:  dark matte top disc (90% diameter) with bold cream pointer line
      from center to edge.
Ring: thin polished chrome bevel between body and cap.
Skirt: subtle knurled texture on cylinder side (vertical grooves).

Lighting: 3-point + bright environment so chrome reflects something.
Output: 60-frame vertical filmstrip 256x256 per frame.

Run: blender -b -P blender_knob_render.py
"""

import bpy
import math
import os

FRAME_SIZE = 256
N_FRAMES   = 60
ROT_START  = math.radians(-135)
ROT_END    = math.radians( 135)
OUT_DIR    = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          '..', 'juce', 'Source', 'assets')
OUT_FRAMES = os.path.join(OUT_DIR, 'tmp_knob_frames')
OUT_FILE   = os.path.join(OUT_DIR, 'knob_chrome_filmstrip.png')

# --- Reset scene ---
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 48
scene.cycles.use_denoising = True
scene.render.resolution_x = FRAME_SIZE
scene.render.resolution_y = FRAME_SIZE
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.film_transparent = True
scene.view_settings.view_transform = 'Standard'

# --- Camera (orthographic top-down with very slight elevation for depth) ---
bpy.ops.object.camera_add(location=(0, -0.05, 4))
cam = bpy.context.object
cam.data.type = 'ORTHO'
cam.data.ortho_scale = 1.32
cam.rotation_euler = (math.radians(2.5), 0, 0)
scene.camera = cam

# ============================================================
#  KNOB BODY — brushed aluminum cylinder (the lower 60% of the knob)
# ============================================================
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=0.50, depth=0.18,
                                    location=(0, 0, 0))
body = bpy.context.object
body.name = 'KnobBody'

# Bevel top edge
mb = body.modifiers.new('Bevel', 'BEVEL')
mb.width = 0.018
mb.segments = 6
mb.profile = 0.7
mb.affect = 'EDGES'
mb.limit_method = 'ANGLE'
mb.angle_limit = math.radians(30)

mb2 = body.modifiers.new('Subdiv', 'SUBSURF')
mb2.levels = 2
mb2.render_levels = 3

# Aluminum material with anisotropic brushing
mat_alu = bpy.data.materials.new('BrushedAlu')
mat_alu.use_nodes = True
nt = mat_alu.node_tree
for n in list(nt.nodes):
    nt.nodes.remove(n)
out = nt.nodes.new('ShaderNodeOutputMaterial'); out.location = (700, 0)
bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled'); bsdf.location = (400, 0)
bsdf.inputs['Base Color'].default_value = (0.86, 0.84, 0.80, 1.0)
bsdf.inputs['Metallic'].default_value = 1.0
bsdf.inputs['Roughness'].default_value = 0.32
# Anisotropy via Anisotropic input on Principled
if 'Anisotropic' in bsdf.inputs:
    bsdf.inputs['Anisotropic'].default_value = 0.55
# Brushing pattern via noise texture modulating roughness
tc = nt.nodes.new('ShaderNodeTexCoord'); tc.location = (-300, -200)
noise = nt.nodes.new('ShaderNodeTexNoise'); noise.location = (-100, -200)
noise.inputs['Scale'].default_value = 250.0
noise.inputs['Detail'].default_value = 1.0
nt.links.new(tc.outputs['Generated'], noise.inputs['Vector'])
ramp = nt.nodes.new('ShaderNodeValToRGB'); ramp.location = (150, -200)
ramp.color_ramp.elements[0].position = 0.40
ramp.color_ramp.elements[1].position = 0.60
ramp.color_ramp.elements[0].color = (0.20, 0.20, 0.20, 1)
ramp.color_ramp.elements[1].color = (0.42, 0.42, 0.42, 1)
nt.links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
nt.links.new(ramp.outputs['Color'], bsdf.inputs['Roughness'])
nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
body.data.materials.append(mat_alu)

# ============================================================
#  KNOB CAP — dark matte disc on top (90% diameter)
# ============================================================
bpy.ops.mesh.primitive_cylinder_add(vertices=96, radius=0.42, depth=0.06,
                                    location=(0, 0, 0.115))
cap = bpy.context.object
cap.name = 'KnobCap'
mc = cap.modifiers.new('Bevel', 'BEVEL')
mc.width = 0.012
mc.segments = 4
mc2 = cap.modifiers.new('Subdiv', 'SUBSURF')
mc2.levels = 2
mc2.render_levels = 3

# Dark matte material for cap
mat_cap = bpy.data.materials.new('DarkCap')
mat_cap.use_nodes = True
pc = mat_cap.node_tree.nodes['Principled BSDF']
pc.inputs['Base Color'].default_value = (0.06, 0.055, 0.05, 1.0)
pc.inputs['Roughness'].default_value = 0.62
pc.inputs['Metallic'].default_value = 0.0
cap.data.materials.append(mat_cap)

# Parent cap to body so they rotate together
cap.parent = body

# ============================================================
#  POINTER LINE — bold cream/white from center to edge of cap
# ============================================================
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.21, 0.155))
pointer = bpy.context.object
pointer.name = 'Pointer'
pointer.scale = (0.025, 0.40, 0.012)

mat_pointer = bpy.data.materials.new('PointerCream')
mat_pointer.use_nodes = True
pp = mat_pointer.node_tree.nodes['Principled BSDF']
pp.inputs['Base Color'].default_value = (0.94, 0.92, 0.84, 1.0)  # cream paint
pp.inputs['Roughness'].default_value = 0.45
pointer.data.materials.append(mat_pointer)
pointer.parent = body

# ============================================================
#  CHROME RING — thin polished bevel between body and cap
# ============================================================
bpy.ops.mesh.primitive_torus_add(major_radius=0.42, minor_radius=0.012,
                                  major_segments=128, minor_segments=12,
                                  location=(0, 0, 0.10))
ring = bpy.context.object
ring.name = 'ChromeRing'
mat_ring = bpy.data.materials.new('PolishedChrome')
mat_ring.use_nodes = True
pr = mat_ring.node_tree.nodes['Principled BSDF']
pr.inputs['Base Color'].default_value = (0.92, 0.90, 0.88, 1.0)
pr.inputs['Metallic'].default_value = 1.0
pr.inputs['Roughness'].default_value = 0.08
ring.data.materials.append(mat_ring)
ring.parent = body

# ============================================================
#  CONCENTRIC GROOVES on body — three subtle rings via array
# ============================================================
for i, z in enumerate([0.04, 0.0, -0.04]):
    bpy.ops.mesh.primitive_torus_add(major_radius=0.50, minor_radius=0.004,
                                      major_segments=128, minor_segments=8,
                                      location=(0, 0, z))
    g = bpy.context.object
    g.name = f'Groove_{i}'
    mat_g = bpy.data.materials.new(f'GrooveDark_{i}')
    mat_g.use_nodes = True
    pg = mat_g.node_tree.nodes['Principled BSDF']
    pg.inputs['Base Color'].default_value = (0.18, 0.17, 0.16, 1.0)
    pg.inputs['Metallic'].default_value = 0.6
    pg.inputs['Roughness'].default_value = 0.4
    g.data.materials.append(mat_g)
    g.parent = body

# ============================================================
#  BLACK BEZEL UNDER KNOB — subtle dark ring at base
# ============================================================
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=0.55, depth=0.04,
                                    location=(0, 0, -0.11))
bezel = bpy.context.object
bezel.name = 'OuterBezel'
mat_bezel = bpy.data.materials.new('BlackBezel')
mat_bezel.use_nodes = True
pb = mat_bezel.node_tree.nodes['Principled BSDF']
pb.inputs['Base Color'].default_value = (0.04, 0.04, 0.04, 1.0)
pb.inputs['Roughness'].default_value = 0.7
bezel.data.materials.append(mat_bezel)

# ============================================================
#  LIGHTING — 3-point + studio environment
# ============================================================
def add_area(name, loc, energy, size, rot, color=(1, 1, 1)):
    bpy.ops.object.light_add(type='AREA', location=loc)
    L = bpy.context.object
    L.name = name
    L.data.energy = energy
    L.data.size = size
    L.data.color = color
    L.rotation_euler = rot
    return L

add_area('Key', (-2.0, -2.0, 2.5), 90, 1.5,
         (math.radians(45), math.radians(-25), math.radians(45)),
         (1.0, 0.97, 0.92))
add_area('Fill', (1.5, 1.5, 1.5), 25, 2.0,
         (math.radians(60), math.radians(15), math.radians(-130)),
         (0.85, 0.92, 1.0))
add_area('Rim', (0.0, 0.0, 3.5), 35, 1.0,
         (0, 0, 0),
         (1.0, 1.0, 1.0))

# Bright neutral environment so chrome reflects something
scene.world = bpy.data.worlds.new('Studio')
scene.world.use_nodes = True
bg = scene.world.node_tree.nodes['Background']
bg.inputs['Color'].default_value = (0.85, 0.85, 0.88, 1.0)
bg.inputs['Strength'].default_value = 1.5

# ============================================================
#  RENDER FRAMES — rotate the body (everything parented follows)
# ============================================================
os.makedirs(OUT_FRAMES, exist_ok=True)
print(f"Rendering {N_FRAMES} premium-knob frames @ {FRAME_SIZE}x{FRAME_SIZE}...")
for i in range(N_FRAMES):
    t = i / (N_FRAMES - 1)
    angle = ROT_START + t * (ROT_END - ROT_START)
    body.rotation_euler = (0, 0, angle)
    fp = os.path.join(OUT_FRAMES, f'frame_{i:03d}.png')
    scene.render.filepath = fp
    bpy.ops.render.render(write_still=True)
    if i % 10 == 0:
        print(f"  frame {i:02d}/{N_FRAMES}  @  {math.degrees(angle):+.1f}°")

print("Compositing filmstrip...")
print(f"Run: python3 -c \"<composite>\"")
