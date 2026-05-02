"""
Blender PBR render of two tileable panel textures:
  - walnut_bezel.png : 1024x512, walnut wood with real grain (Voronoi + Noise),
                      slight gloss varnish, bevel highlight edge.
  - matte_panel.png  : 1024x512, matte black plastic with subtle pebble texture,
                      tiny bake of upper-left key light edge highlight.

Output replaces the CSS gradient backgrounds for .deck (bezel) and
.deck::before (panel) so wood + panel reads as real material under
the same upper-left key-light convention.
"""
import bpy, math, os

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'juce', 'Source', 'assets')
RES_W, RES_H = 1024, 512


def reset(samples=64):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.render.engine = 'CYCLES'
    s.cycles.device = 'CPU'
    s.cycles.samples = samples
    s.cycles.use_denoising = True
    s.render.resolution_x = RES_W
    s.render.resolution_y = RES_H
    s.render.resolution_percentage = 100
    s.render.image_settings.file_format = 'PNG'
    s.render.image_settings.color_mode = 'RGB'
    s.render.film_transparent = False
    s.view_settings.view_transform = 'Standard'
    return s


def add_camera_top():
    bpy.ops.object.camera_add(location=(0, 0, 4))
    cam = bpy.context.object
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 4.0  # 4 units wide horizontally
    bpy.context.scene.camera = cam


def add_lights():
    # Key from upper-left @ 45°
    bpy.ops.object.light_add(type='AREA', location=(-3, -2, 3))
    L = bpy.context.object
    L.data.energy = 100
    L.data.size = 4.0
    L.data.color = (1.0, 0.97, 0.92)
    L.rotation_euler = (math.radians(45), math.radians(-25), math.radians(45))
    # Fill from lower-right
    bpy.ops.object.light_add(type='AREA', location=(2.5, 2.5, 1.5))
    F = bpy.context.object
    F.data.energy = 25
    F.data.size = 4.0
    F.data.color = (0.85, 0.92, 1.0)
    F.rotation_euler = (math.radians(60), math.radians(15), math.radians(-130))


def make_walnut_material():
    m = bpy.data.materials.new('Walnut')
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new('ShaderNodeOutputMaterial'); out.location = (900, 0)
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled'); bsdf.location = (600, 0)
    bsdf.inputs['Roughness'].default_value = 0.45
    bsdf.inputs['Coat Weight'].default_value = 0.20
    bsdf.inputs['Coat Roughness'].default_value = 0.10
    # Texture coords
    tc = nt.nodes.new('ShaderNodeTexCoord'); tc.location = (-700, 0)
    map_ = nt.nodes.new('ShaderNodeMapping'); map_.location = (-500, 0)
    map_.inputs['Scale'].default_value[0] = 0.5  # stretch grain horizontally
    map_.inputs['Scale'].default_value[1] = 4.0
    nt.links.new(tc.outputs['Generated'], map_.inputs['Vector'])
    # Voronoi + Noise grain
    voro = nt.nodes.new('ShaderNodeTexVoronoi'); voro.location = (-300, 100)
    voro.inputs['Scale'].default_value = 12.0
    nt.links.new(map_.outputs['Vector'], voro.inputs['Vector'])
    noise = nt.nodes.new('ShaderNodeTexNoise'); noise.location = (-300, -100)
    noise.inputs['Scale'].default_value = 80.0
    noise.inputs['Detail'].default_value = 4.0
    noise.inputs['Roughness'].default_value = 0.6
    nt.links.new(map_.outputs['Vector'], noise.inputs['Vector'])
    mix = nt.nodes.new('ShaderNodeMixRGB'); mix.location = (-50, 0)
    mix.blend_type = 'MULTIPLY'
    mix.inputs['Fac'].default_value = 0.6
    nt.links.new(voro.outputs['Distance'], mix.inputs['Color1'])
    nt.links.new(noise.outputs['Color'], mix.inputs['Color2'])
    ramp = nt.nodes.new('ShaderNodeValToRGB'); ramp.location = (200, 0)
    ramp.color_ramp.elements[0].color = (0.10, 0.045, 0.020, 1.0)  # dark walnut
    ramp.color_ramp.elements[1].color = (0.32, 0.16, 0.07, 1.0)    # mid walnut
    ramp.color_ramp.elements[0].position = 0.30
    ramp.color_ramp.elements[1].position = 0.85
    nt.links.new(mix.outputs['Color'], ramp.inputs['Fac'])
    nt.links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    # Slight bump from grain
    bump = nt.nodes.new('ShaderNodeBump'); bump.location = (400, -200)
    bump.inputs['Strength'].default_value = 0.15
    nt.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    nt.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return m


def make_matte_panel_material():
    m = bpy.data.materials.new('MattePlastic')
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (0.06, 0.055, 0.05, 1.0)  # near-black
    bsdf.inputs['Roughness'].default_value = 0.78
    bsdf.inputs['Metallic'].default_value = 0.0
    # Pebble bump for plastic feel
    tc = nt.nodes.new('ShaderNodeTexCoord'); tc.location = (-600, -200)
    noise = nt.nodes.new('ShaderNodeTexNoise'); noise.location = (-300, -200)
    noise.inputs['Scale'].default_value = 200.0
    noise.inputs['Detail'].default_value = 6.0
    nt.links.new(tc.outputs['Generated'], noise.inputs['Vector'])
    bump = nt.nodes.new('ShaderNodeBump'); bump.location = (50, -200)
    bump.inputs['Strength'].default_value = 0.05
    nt.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    nt.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    return m


def render_plane_with_material(material_factory, out_filename):
    s = reset(samples=128)
    add_camera_top()
    add_lights()
    # Build material AFTER reset (factory_settings wipe)
    mat = material_factory()
    bpy.ops.mesh.primitive_plane_add(size=4, location=(0, 0, 0))
    plane = bpy.context.object
    plane.scale = (1.0, 0.5, 1.0)  # 4w x 2h to match resolution aspect
    plane.data.materials.append(mat)

    fp = os.path.join(OUT_DIR, out_filename)
    s.render.filepath = fp
    bpy.ops.render.render(write_still=True)
    print(f"Wrote: {fp}")


print("=== Rendering walnut bezel ===")
render_plane_with_material(make_walnut_material, 'walnut_bezel.png')

print("\n=== Rendering matte panel ===")
render_plane_with_material(make_matte_panel_material, 'matte_panel.png')

print("\nDone.")
