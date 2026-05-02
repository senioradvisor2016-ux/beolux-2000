"""
Blender headless renderer for chrome multi-band fader thumb.
Single 64x32 PNG with proper PBR baked lighting.
"""
import bpy, math, os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   '..', 'juce', 'Source', 'assets', 'fader_thumb_chrome.png')

bpy.ops.wm.read_factory_settings(use_empty=True)
s = bpy.context.scene
s.render.engine = 'CYCLES'
s.cycles.device = 'CPU'
s.cycles.samples = 64
s.cycles.use_denoising = True
s.render.resolution_x = 256
s.render.resolution_y = 128
s.render.image_settings.file_format = 'PNG'
s.render.image_settings.color_mode = 'RGBA'
s.render.film_transparent = True
s.view_settings.view_transform = 'Standard'

# Camera
bpy.ops.object.camera_add(location=(0, 0, 4))
cam = bpy.context.object
cam.data.type = 'ORTHO'
cam.data.ortho_scale = 2.4
s.camera = cam

# Thumb body — flat box with rounded top
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0))
thumb = bpy.context.object
thumb.scale = (1.0, 0.5, 0.18)
mod = thumb.modifiers.new('Bevel', 'BEVEL')
mod.width = 0.04; mod.segments = 6
mod = thumb.modifiers.new('Subdiv', 'SUBSURF')
mod.levels = 2; mod.render_levels = 3

# Multi-band horizontal grooves via array of thin cutters
for i, y in enumerate([-0.30, -0.15, 0.0, 0.15, 0.30]):
    bpy.ops.mesh.primitive_cube_add(size=0.02, location=(0, y, 0.20))
    g = bpy.context.object
    g.scale = (50, 0.4, 0.5)
    bm = thumb.modifiers.new(f'Groove{i}', 'BOOLEAN')
    bm.object = g
    bm.operation = 'DIFFERENCE'
    g.hide_render = True

# Chrome material
m = bpy.data.materials.new('Chrome')
m.use_nodes = True
p = m.node_tree.nodes['Principled BSDF']
p.inputs['Base Color'].default_value = (0.78, 0.76, 0.72, 1.0)
p.inputs['Metallic'].default_value = 1.0
p.inputs['Roughness'].default_value = 0.16
thumb.data.materials.append(m)

# Lighting (key upper-left)
def add_area(loc, energy, size, rot, color=(1,1,1)):
    bpy.ops.object.light_add(type='AREA', location=loc)
    L = bpy.context.object
    L.data.energy = energy
    L.data.size = size
    L.data.color = color
    L.rotation_euler = rot
add_area((-2, -1.5, 2.5), 80, 1.8, (math.radians(45), math.radians(-25), math.radians(45)), (1.0, 0.97, 0.92))
add_area(( 1.5, 1.5, 1.5), 25, 2.0, (math.radians(60), math.radians(15), math.radians(-130)), (0.85, 0.92, 1.0))

# World transparent
s.world = bpy.data.worlds.new('Empty')
s.world.use_nodes = True
bg = s.world.node_tree.nodes['Background']
bg.inputs['Color'].default_value = (0, 0, 0, 0)
bg.inputs['Strength'].default_value = 0.0

s.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print(f"Wrote: {OUT}")
