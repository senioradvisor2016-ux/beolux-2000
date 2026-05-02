"""
Blender headless renderer for BC2000DL chrome knob filmstrip.

Run: blender -b -P blender_knob_render.py

Generates a 60-frame filmstrip PNG (frames stacked vertically) at 256x256 per frame.
Uses Cycles renderer with PBR chrome material:
  - Base colour: cool brushed steel #b8b4a8
  - Metallic: 1.0
  - Roughness: 0.18 (slight brushed anisotropy via noise texture)
  - Concentric grooves via subtle displacement
  - Pointer notch as boolean cut on top surface

Lighting (3-point, upper-left key matching plugin convention):
  - Key:  area light, 8 W, 45° upper-left, soft
  - Fill: point light, 2 W, lower-right (cool tint #c8d4e0)
  - Rim:  area light, 4 W, behind, top
"""

import bpy
import math
import os
import sys

# --- Config ---
FRAME_SIZE = 256
N_FRAMES   = 60
ROT_START  = math.radians(-135)
ROT_END    = math.radians( 135)
OUT_DIR    = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          '..', 'juce', 'Source', 'assets')
OUT_FRAMES = os.path.join(OUT_DIR, 'tmp_knob_frames')
OUT_FILE   = os.path.join(OUT_DIR, 'knob_chrome_filmstrip_3d.png')

# --- Reset scene ---
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# Renderer
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 32
scene.cycles.use_denoising = True
scene.render.resolution_x = FRAME_SIZE
scene.render.resolution_y = FRAME_SIZE
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.film_transparent = True
scene.view_settings.view_transform = 'Standard'

# --- Camera (orthographic top-down with slight perspective for depth feel) ---
bpy.ops.object.camera_add(location=(0, 0, 4))
cam = bpy.context.object
cam.data.type = 'ORTHO'
cam.data.ortho_scale = 1.4
scene.camera = cam

# --- Knob mesh: cylinder with bevel for chrome dome ---
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=0.5, depth=0.18,
                                    location=(0, 0, 0))
knob = bpy.context.object
knob.name = 'Knob'

# Bevel the top edge for chrome dome shape
mod = knob.modifiers.new('Bevel', 'BEVEL')
mod.width = 0.06
mod.segments = 8
mod.profile = 0.7
mod.affect = 'EDGES'
mod.limit_method = 'ANGLE'
mod.angle_limit = math.radians(30)

# Subdivision for smoother surface
sub = knob.modifiers.new('Subdiv', 'SUBSURF')
sub.levels = 2
sub.render_levels = 3

# Pointer notch (small slot cut on top)
bpy.ops.mesh.primitive_cube_add(size=0.04, location=(0, 0.30, 0.10))
notch = bpy.context.object
notch.name = 'NotchCutter'
notch.scale = (0.6, 4.5, 1.5)
boolmod = knob.modifiers.new('NotchBool', 'BOOLEAN')
boolmod.object = notch
boolmod.operation = 'DIFFERENCE'
notch.hide_viewport = True
notch.hide_render = True

# Make notch a child of knob so it rotates with the knob
notch.parent = knob

# --- Material: PBR chrome with slight anisotropic brushing ---
mat = bpy.data.materials.new('ChromeBrushed')
mat.use_nodes = True
nt = mat.node_tree
for n in list(nt.nodes):
    nt.nodes.remove(n)
out = nt.nodes.new('ShaderNodeOutputMaterial'); out.location = (700, 0)
bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled'); bsdf.location = (400, 0)
bsdf.inputs['Base Color'].default_value = (0.78, 0.76, 0.72, 1.0)
bsdf.inputs['Metallic'].default_value = 1.0
bsdf.inputs['Roughness'].default_value = 0.18
# Subtle noise on roughness for brushed feel
noise = nt.nodes.new('ShaderNodeTexNoise'); noise.location = (-100, -200)
noise.inputs['Scale'].default_value = 80.0
noise.inputs['Detail'].default_value = 2.0
ramp = nt.nodes.new('ShaderNodeValToRGB'); ramp.location = (150, -200)
ramp.color_ramp.elements[0].position = 0.45
ramp.color_ramp.elements[1].position = 0.55
ramp.color_ramp.elements[0].color = (0.13, 0.13, 0.13, 1)
ramp.color_ramp.elements[1].color = (0.22, 0.22, 0.22, 1)
nt.links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
nt.links.new(ramp.outputs['Color'], bsdf.inputs['Roughness'])
nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
knob.data.materials.append(mat)

# Black bezel ring under the knob
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=0.55, depth=0.04,
                                    location=(0, 0, -0.11))
bezel = bpy.context.object
bezel.name = 'Bezel'
bm = bpy.data.materials.new('Bezel')
bm.use_nodes = True
bm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (0.05, 0.05, 0.05, 1)
bm.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.6
bezel.data.materials.append(bm)

# --- Lighting: 3-point, upper-left key ---
def add_area(name, loc, energy, size, rot, color=(1,1,1)):
    bpy.ops.object.light_add(type='AREA', location=loc)
    L = bpy.context.object
    L.name = name
    L.data.energy = energy
    L.data.size = size
    L.data.color = color
    L.rotation_euler = rot
    return L

# Key from upper-left @ 45°
add_area('Key',  (-2.0, -2.0, 2.5), 80, 1.5,
         (math.radians(45), math.radians(-25), math.radians(45)),
         (1.0, 0.97, 0.92))
# Fill from lower-right (cooler)
add_area('Fill', ( 1.5,  1.5, 1.5), 25, 2.0,
         (math.radians(60), math.radians(15), math.radians(-130)),
         (0.85, 0.92, 1.0))
# Rim/back top
add_area('Rim',  ( 0.0,  0.0, 3.5), 30, 1.0,
         (0, 0, 0),
         (1.0, 1.0, 1.0))

# --- World: bright neutral environment so chrome has something to reflect.
# (Without this, mirror-finish metal renders pitch black where direct lights
# don't hit. We keep film_transparent=True so output PNG alpha is preserved.) ---
scene.world = bpy.data.worlds.new('Studio')
scene.world.use_nodes = True
bg = scene.world.node_tree.nodes['Background']
bg.inputs['Color'].default_value = (0.85, 0.85, 0.88, 1.0)  # cool neutral grey
bg.inputs['Strength'].default_value = 1.6

# --- Render frames ---
os.makedirs(OUT_FRAMES, exist_ok=True)
print(f"Rendering {N_FRAMES} frames @ {FRAME_SIZE}x{FRAME_SIZE}...")
for i in range(N_FRAMES):
    t = i / (N_FRAMES - 1)
    angle = ROT_START + t * (ROT_END - ROT_START)
    knob.rotation_euler = (0, 0, angle)
    fp = os.path.join(OUT_FRAMES, f'frame_{i:03d}.png')
    scene.render.filepath = fp
    bpy.ops.render.render(write_still=True)
    if i % 10 == 0:
        print(f"  frame {i:02d}/{N_FRAMES}  @  {math.degrees(angle):+.1f}°")

# --- Composite frames into vertical filmstrip via PIL ---
print("Compositing filmstrip...")
from PIL import Image
strip = Image.new('RGBA', (FRAME_SIZE, FRAME_SIZE * N_FRAMES), (0, 0, 0, 0))
for i in range(N_FRAMES):
    fp = os.path.join(OUT_FRAMES, f'frame_{i:03d}.png')
    f = Image.open(fp).convert('RGBA')
    strip.paste(f, (0, i * FRAME_SIZE), f)
strip.save(OUT_FILE, 'PNG', optimize=True)
print(f"\nWrote: {OUT_FILE}")
print(f"Size:  {FRAME_SIZE}x{FRAME_SIZE * N_FRAMES} ({N_FRAMES} frames)")
