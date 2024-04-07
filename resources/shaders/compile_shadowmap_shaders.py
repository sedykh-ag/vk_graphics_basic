import os
import subprocess
import pathlib

if __name__ == '__main__':
    glslang_cmd = "glslangValidator"

    shader_list = [
      "simple.vert",
      "quad.vert",
      "quad.frag",
      "simple_shadow.frag",
      "jitter.vert",
      "blend.frag",
      "quad3_vert.vert",
      "quad3.vert",
      "motion_vector.frag",
    ]

    for shader in shader_list:
        subprocess.run([glslang_cmd, "-V", shader, "-o", "{}.spv".format(shader)])

