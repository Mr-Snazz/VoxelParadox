#version 460 core
// block.frag
// Main block surface shader. Builds the procedural material, applies biome tint,
// lighting, fog, transparency, and the block breaking effect.

in vec3 vWorldPos;
in vec3 vLocalPos;
in vec3 vNormal;
in vec3 vFaceNormal;
in vec4 vTint;
in float vAO;
flat in int vMaterialId;

uniform vec3 uCameraPos;
uniform vec4 uFogColor;
uniform float uFogDensity;
uniform float uTime;
uniform float uAlpha;
uniform float uAoStrength;
uniform vec4 uBiomeTint;
uniform int uUseLocalMaterialSpace;
uniform vec3 uBreakBlockCenter;
uniform float uBreakProgress;
uniform vec3 uHighlightBlockCenter;
uniform float uHighlightActive;

out vec4 FragColor;

const int MATERIAL_STONE = 1;
const int MATERIAL_CRYSTAL = 2;
const int MATERIAL_VOID_MATTER = 3;
const int MATERIAL_MEMBRANE = 4;
const int MATERIAL_ORGANIC = 5;
const int MATERIAL_METAL = 6;
const int MATERIAL_PORTAL = 7;
const int MATERIAL_MEMBRANE_WEAVE = 8;

struct MaterialSample {
    vec3 albedo;
    float roughness;
    float specular;
    float emissive;
};

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float hash31(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

float noise21(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm21(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 4; octave++) {
        value += noise21(p) * amplitude;
        p = p * 2.03 + vec2(19.7, 11.3);
        amplitude *= 0.5;
    }
    return value;
}

const float BREAK_PHASE_POWER = 2.0;
const float BREAK_PIXEL_GRID = 16.0;
const float BREAK_STAR_GRID = 25.0;
const float BREAK_CORNER_MAX_DISTANCE = 0.70710678;
const float HIGHLIGHT_EDGE_INNER = 0.018;
const float HIGHLIGHT_EDGE_MID = 0.015;
const float HIGHLIGHT_EDGE_OUTER = 0.010;
const float HIGHLIGHT_ANTS_SCALE = 0.08;
const float HIGHLIGHT_ANTS_SPEED = 1.6;

vec2 breakHash2(vec2 p) {
    return vec2(
        hash21(p),
        hash21(p + vec2(37.1, 91.7))
    );
}

float breakFaceId(vec3 normal) {
    vec3 absNormal = abs(normal);
    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        return normal.x > 0.0 ? 0.0 : 1.0;
    }
    if (absNormal.z > absNormal.y) {
        return normal.z > 0.0 ? 2.0 : 3.0;
    }
    return normal.y > 0.0 ? 4.0 : 5.0;
}

// The old effect evaluated a full 3D Voronoi with two expensive passes per
// fragment. The fracture is only visible on the current face, so a 2D Worley
// pattern in face space keeps the same visual language at a much lower cost.
vec3 breakVoronoi2D(vec2 x) {
    vec2 n = floor(x);
    vec2 f = fract(x);
    vec2 mg = vec2(0.0);
    vec2 mr = vec2(0.0);

    float md = 1e9;

    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = breakHash2(n + g);
            vec2 r = g + o - f;
            float d = dot(r, r);

            if (d < md) {
                md = d;
                mr = r;
                mg = g;
            }
        }
    }

    md = 1e9;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            vec2 g = mg + vec2(float(i), float(j));
            vec2 o = breakHash2(n + g);
            vec2 r = g + o - f;
            vec2 delta = r - mr;
            float deltaLengthSq = dot(delta, delta);

            if (deltaLengthSq > 0.00001) {
                md = min(md, dot(0.5 * (mr + r), delta * inversesqrt(deltaLengthSq)));
            }
        }
    }

    return vec3(md, n + mg);
}

vec2 faceUv(vec3 worldPos, vec3 normal) {
    vec3 absNormal = abs(normal);

    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        return vec2(worldPos.z, worldPos.y);
    }

    if (absNormal.z > absNormal.y) {
        return vec2(worldPos.x, worldPos.y);
    }

    return vec2(worldPos.x, worldPos.z);
}

float blockMask(vec3 worldPos, vec3 blockCenter) {
    vec3 blockMin = blockCenter - vec3(0.5);
    vec3 blockMax = blockCenter + vec3(0.5);
    vec3 inside =
        step(blockMin - vec3(0.0015), worldPos) *
        step(worldPos, blockMax + vec3(0.0015));
    return inside.x * inside.y * inside.z;
}

// Photoshop-style block selection outline:
// this runs only for the highlighted block and keeps the effect on the cube
// silhouette by checking whether each face edge borders a neighbor face that is
// turned away from the camera.
vec3 applySelectionHighlight(vec3 baseColor, vec3 worldPos, vec3 normal) {
    float selectionMask = uHighlightActive * blockMask(worldPos, uHighlightBlockCenter);
    if (selectionMask <= 0.0) {
        return baseColor;
    }

    vec3 viewDir = normalize(uCameraPos - worldPos);
    vec3 local = clamp(worldPos - (uHighlightBlockCenter - vec3(0.5)), 0.0, 1.0);
    vec2 localUv = clamp(faceUv(local, normal), 0.0, 1.0);
    float aa = max(max(fwidth(localUv.x), fwidth(localUv.y)) * 1.5, 0.0005);

    vec3 absNormal = abs(normal);
    vec3 edgeNormalMinU = vec3(0.0);
    vec3 edgeNormalMaxU = vec3(0.0);
    vec3 edgeNormalMinV = vec3(0.0);
    vec3 edgeNormalMaxV = vec3(0.0);

    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        edgeNormalMinU = vec3(0.0, 0.0, -1.0);
        edgeNormalMaxU = vec3(0.0, 0.0, 1.0);
        edgeNormalMinV = vec3(0.0, -1.0, 0.0);
        edgeNormalMaxV = vec3(0.0, 1.0, 0.0);
    } else if (absNormal.z > absNormal.y) {
        edgeNormalMinU = vec3(-1.0, 0.0, 0.0);
        edgeNormalMaxU = vec3(1.0, 0.0, 0.0);
        edgeNormalMinV = vec3(0.0, -1.0, 0.0);
        edgeNormalMaxV = vec3(0.0, 1.0, 0.0);
    } else {
        edgeNormalMinU = vec3(-1.0, 0.0, 0.0);
        edgeNormalMaxU = vec3(1.0, 0.0, 0.0);
        edgeNormalMinV = vec3(0.0, 0.0, -1.0);
        edgeNormalMaxV = vec3(0.0, 0.0, 1.0);
    }

    float silhouetteMinU = 1.0 - smoothstep(-0.02, 0.04, dot(edgeNormalMinU, viewDir));
    float silhouetteMaxU = 1.0 - smoothstep(-0.02, 0.04, dot(edgeNormalMaxU, viewDir));
    float silhouetteMinV = 1.0 - smoothstep(-0.02, 0.04, dot(edgeNormalMinV, viewDir));
    float silhouetteMaxV = 1.0 - smoothstep(-0.02, 0.04, dot(edgeNormalMaxV, viewDir));

    float outerMinU =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_MID - aa, HIGHLIGHT_EDGE_OUTER + aa, localUv.x)) *
        silhouetteMinU;
    float outerMaxU =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_MID - aa, HIGHLIGHT_EDGE_OUTER + aa,
                          1.0 - localUv.x)) * silhouetteMaxU;
    float outerMinV =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_MID - aa, HIGHLIGHT_EDGE_OUTER + aa, localUv.y)) *
        silhouetteMinV;
    float outerMaxV =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_MID - aa, HIGHLIGHT_EDGE_OUTER + aa,
                          1.0 - localUv.y)) * silhouetteMaxV;

    float innerMinU =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_INNER - aa, HIGHLIGHT_EDGE_MID + aa, localUv.x)) *
        silhouetteMinU;
    float innerMaxU =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_INNER - aa, HIGHLIGHT_EDGE_MID + aa,
                          1.0 - localUv.x)) * silhouetteMaxU;
    float innerMinV =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_INNER - aa, HIGHLIGHT_EDGE_MID + aa, localUv.y)) *
        silhouetteMinV;
    float innerMaxV =
        (1.0 - smoothstep(HIGHLIGHT_EDGE_INNER - aa, HIGHLIGHT_EDGE_MID + aa,
                          1.0 - localUv.y)) * silhouetteMaxV;

    float outerBand = max(max(outerMinU, outerMaxU), max(outerMinV, outerMaxV));
    float innerBand = max(max(innerMinU, innerMaxU), max(innerMinV, innerMaxV));

    float pattern = fract((gl_FragCoord.x + gl_FragCoord.y) * HIGHLIGHT_ANTS_SCALE -
                          uTime * HIGHLIGHT_ANTS_SPEED);
    float ants = step(0.5, pattern);
    vec3 antsColor = mix(vec3(0.02), vec3(1.0), ants);

    vec3 color = mix(baseColor, vec3(0.0), clamp(outerBand * 0.95 * selectionMask, 0.0, 1.0));
    color = mix(color, antsColor, clamp(innerBand * selectionMask, 0.0, 1.0));
    return color;
}

vec3 blockBaseColor(int materialId) {
    if (materialId == MATERIAL_STONE) {
        return vec3(0.45, 0.42, 0.50);
    }
    if (materialId == MATERIAL_CRYSTAL) {
        return vec3(0.15, 0.85, 0.95);
    }
    if (materialId == MATERIAL_VOID_MATTER) {
        return vec3(0.20, 0.08, 0.30);
    }
    if (materialId == MATERIAL_MEMBRANE) {
        return vec3(0.25, 0.90, 0.55);
    }
    if (materialId == MATERIAL_ORGANIC) {
        return vec3(0.75, 0.35, 0.28);
    }
    if (materialId == MATERIAL_METAL) {
        return vec3(0.68, 0.70, 0.75);
    }
    if (materialId == MATERIAL_PORTAL) {
        return vec3(0.95, 0.20, 0.85);
    }
    if (materialId == MATERIAL_MEMBRANE_WEAVE) {
        return vec3(0.58, 0.92, 0.78);
    }
    return vec3(1.0, 0.0, 1.0);
}

MaterialSample makeSample(vec3 albedo, float roughness, float specular, float emissive) {
    MaterialSample result;
    result.albedo = albedo;
    result.roughness = roughness;
    result.specular = specular;
    result.emissive = emissive;
    return result;
}

MaterialSample sampleBlockMaterial(int materialId, vec3 worldPos, vec3 worldNormal,
                                   vec3 faceNormal, vec3 viewDir) {
    vec3 base = blockBaseColor(materialId);
    vec2 uv = faceUv(worldPos, faceNormal);
    vec2 localUv = fract(uv + vec2(0.001));
    vec2 centeredUv = localUv - 0.5;
    vec3 cell = floor(worldPos - faceNormal * 0.5 + vec3(0.001));
    float cellHash = hash31(cell + vec3(float(materialId) * 1.6180339, 2.13, 4.37));

    if (materialId == MATERIAL_STONE) {
        float strata = fbm21(vec2(uv.x * 3.4 + cellHash * 4.0, worldPos.y * 0.28 + uv.y * 1.4));
        float grain = noise21(localUv * 11.0 + cellHash * 9.0);
        float cracks = smoothstep(0.63, 0.82,
                                  fbm21(uv * 8.0 + vec2(cellHash * 17.0, -cellHash * 13.0)));
        vec3 albedo = base * mix(0.72, 1.15, strata);
        albedo *= mix(0.96, 0.82, cracks * 0.55);
        albedo += vec3((grain - 0.5) * 0.06);
        return makeSample(clamp(albedo, vec3(0.0), vec3(1.4)), 0.92, 0.05, 0.0);
    }

    if (materialId == MATERIAL_CRYSTAL) {
        float bands = 0.5 + 0.5 * sin((uv.x + uv.y) * 18.0 + uTime * 2.4);
        float sparkle = noise21(localUv * 14.0 + cellHash * 9.0);
        float fresnel = pow(1.0 - max(dot(worldNormal, viewDir), 0.0), 4.0);
        vec3 albedo = mix(base * 0.78, vec3(1.0), bands * 0.32 + fresnel * 0.22);
        albedo *= 0.96 + sparkle * 0.08;
        albedo += vec3(0.08, 0.14, 0.18) * fresnel;

        return makeSample(clamp(albedo, vec3(0.0), vec3(1.6)), 0.14, 0.65,
                          0.42 + bands * 0.28 + fresnel * 0.30 + sparkle * 0.04);
    }

    if (materialId == MATERIAL_VOID_MATTER) {
        float voidNoise = fbm21(uv * 6.0 + vec2(uTime * 0.20, -uTime * 0.20) + cellHash * 5.0);
        float wisps = 0.5 + 0.5 * sin((uv.x - uv.y) * 14.0 - uTime * 3.0 + cellHash * 8.0);
        float rim = pow(1.0 - max(dot(worldNormal, viewDir), 0.0), 2.4);
        vec3 albedo = mix(base * 0.22, base * 0.85 + vec3(0.08, 0.0, 0.12), voidNoise);
        albedo += vec3(0.06, 0.01, 0.08) * wisps * 0.25;
        return makeSample(clamp(albedo, vec3(0.0), vec3(1.2)), 0.52, 0.18,
                          0.12 + rim * 0.18 + wisps * 0.08);
    }

    if (materialId == MATERIAL_MEMBRANE) {
        float veins = fbm21(vec2(uv.x * 5.0, uv.y * 1.0 - uTime * 0.45));
        float ridge = abs(sin(uv.x * 18.0 + veins * 6.0 + uTime * 2.1));
        float pulseField = fbm21(uv * 0.35 + vec2(3.7, 8.1));
        float pulse = 0.5 + 0.5 * sin(uTime * 5.8 + pulseField * 6.2831853);
        float pores = noise21(localUv * 18.0 + cellHash * 13.0);

        vec3 albedo = mix(base * 0.72, base * 1.18, smoothstep(0.36, 0.92, ridge));
        albedo *= 0.95 + pores * 0.08;
        albedo += vec3(0.06, 0.10, 0.05) * pulse * 0.18;

        return makeSample(clamp(albedo, vec3(0.0), vec3(1.4)), 0.66, 0.12,
                          0.10 + ridge * 0.10 * pulse);
    }

    if (materialId == MATERIAL_ORGANIC) {
        float fiber = fbm21(vec2(uv.x * 9.0 + cellHash * 4.0, uv.y * 3.0 - cellHash * 3.0));
        float pores = noise21(localUv * 18.0 + cellHash * 13.0);
        vec3 albedo = mix(base * 0.78, base * 1.06, fiber);
        albedo *= 0.92 + pores * 0.10;
        albedo += vec3(0.05, 0.02, 0.01) * smoothstep(0.72, 1.0, pores);
        return makeSample(clamp(albedo, vec3(0.0), vec3(1.25)), 0.78, 0.08, 0.0);
    }

    if (materialId == MATERIAL_METAL) {
        float brushed = 0.5 + 0.5 * sin(uv.y * 96.0);
        float scratches = fbm21(vec2(uv.x * 18.0, uv.y * 72.0));
        float microScratch = noise21(localUv * 18.0 + cellHash * 7.0);
        float edge = pow(1.0 - max(dot(worldNormal, viewDir), 0.0), 3.5);
        vec3 albedo = mix(base * 0.72, base * 1.12,
                          brushed * 0.22 + scratches * 0.14 + microScratch * 0.04);
        albedo += vec3(edge) * 0.08;

        return makeSample(clamp(albedo, vec3(0.0), vec3(1.6)), 0.18, 0.85, 0.0);
    }

    if (materialId == MATERIAL_PORTAL) {
        float dist = length(centeredUv);
        float ring = 0.5 + 0.5 * sin(dist * 26.0 - uTime * 2.0);
        float vortex = fbm21(uv * 2.25 + vec2(uTime * 0.15, -uTime * 0.12) +
                             vec2(cellHash * 9.0));
        vec3 albedo = mix(vec3(0.06, 0.02, 0.10),
                          base * 1.20 + vec3(0.10, 0.0, 0.16), vortex);
        albedo += vec3(0.18, 0.03, 0.22) * ring * 0.25;
        return makeSample(clamp(albedo, vec3(0.0), vec3(1.8)), 0.05, 0.35,
                          0.55 + ring * 2.30 + vortex * 0.20);
    }

    if (materialId == MATERIAL_MEMBRANE_WEAVE) {
        float weaveA = smoothstep(0.5, 0.58, abs(sin(localUv.x * 16.0 + 1.5)));
        float weaveB = smoothstep(0.5, 0.58, abs(sin(localUv.y * 16.0 + 1.5)));
        float knots = fbm21(localUv * 16.0 + cellHash * 5.0);
        float weave = max(weaveA, weaveB);
        vec3 albedo = mix(base * 0.72, base * 1.10, weave);
        albedo *= 0.90 + knots * 0.12;
        return makeSample(clamp(albedo, vec3(0.0), vec3(1.4)), 0.52, 0.22,
                          0.08 * weave);
    }

    return makeSample(base, 0.80, 0.10, 0.0);
}

float bayerDither4x4(vec2 pix, float brightness) {
    int x = int(mod(pix.x, 4.0));
    int y = int(mod(pix.y, 4.0));
    float matrix[16] = float[16](
        0.0,  8.0,  2.0,  10.0,
        12.0, 4.0,  14.0, 6.0,
        3.0,  11.0, 1.0,  9.0,
        15.0, 7.0,  13.0, 5.0
    );
    float threshold = (matrix[y * 4 + x] + 0.5) / 16.0;
    return brightness > threshold ? 1.0 : 0.0;
}

void main() {
    vec3 worldNormal = normalize(vNormal);
    vec3 faceNormal = normalize(vFaceNormal);
    vec3 materialPos = (uUseLocalMaterialSpace != 0) ? vLocalPos : vWorldPos;
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    MaterialSample material =
        sampleBlockMaterial(vMaterialId, materialPos, worldNormal, faceNormal, viewDir);

    vec3 tint = clamp(vTint.rgb * uBiomeTint.rgb, vec3(0.45), vec3(1.85));
    material.albedo *= tint;

    vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(worldNormal, lightDir), 0.0);
    float ambient = 0.38;
    float ao = mix(0.35, 1.0, vAO);
    float light = (ambient + diffuse * 0.55) * mix(1.0, ao, uAoStrength);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specPower = mix(14.0, 96.0, 1.0 - material.roughness);
    float specular =
        pow(max(dot(worldNormal, halfDir), 0.0), specPower) * material.specular;

    vec3 color = material.albedo * light +
                 mix(vec3(specular), material.albedo * specular, 0.25);

    float pulse = 0.5 + 0.5 * sin(uTime * 3.0 + vWorldPos.x + vWorldPos.z);
    vec2 ditherCoord = faceUv(materialPos, faceNormal) * 16.0;
    float ditherMask = bayerDither4x4(floor(ditherCoord), pulse);
    float ditheredPulse = mix(pulse * 0.45, pulse, ditherMask);
    float emissive =
        (material.emissive + vTint.a * (0.4 + 0.6 * ditheredPulse)) * uBiomeTint.a;
    color += material.albedo * emissive;

    if (uBreakProgress > 0.001) {
        vec3 blockMin = uBreakBlockCenter - vec3(0.5);
        vec3 blockMax = uBreakBlockCenter + vec3(0.5);
        vec3 inside =
            step(blockMin - vec3(0.001), vWorldPos) *
            step(vWorldPos, blockMax + vec3(0.001));
        float breakMask = inside.x * inside.y * inside.z;

        if (breakMask > 0.5) {
            vec3 local = clamp(vWorldPos - blockMin, 0.0, 1.0);
            vec3 pixelLocal =
                (floor(local * BREAK_PIXEL_GRID) + vec3(0.5)) / BREAK_PIXEL_GRID;

            vec2 breakUv = faceUv(pixelLocal, faceNormal);
            vec3 blockSeed = floor(uBreakBlockCenter);
            float faceId = breakFaceId(faceNormal);
            vec2 faceSeed = vec2(
                hash31(blockSeed + vec3(11.3, 7.1, 3.7 + faceId)),
                hash31(blockSeed + vec3(23.8, 19.4, 17.2 + faceId * 1.6180339))
            );

            vec2 p = (breakUv - 0.5) * 12.0 + faceSeed * 8.0;
            vec3 c = breakVoronoi2D(p);
            float edgeDistance = c.x;
            vec2 nearestCell = c.yz;

            vec2 cornerDelta = min(breakUv, vec2(1.0) - breakUv);
            float cornerDistance = length(cornerDelta);
            float propagation =
                pow(clamp(cornerDistance / BREAK_CORNER_MAX_DISTANCE, 0.0, 1.0),
                    BREAK_PHASE_POWER);

            float cellOffset =
                (hash21(nearestCell + faceSeed * 13.0 +
                        vec2(faceId * 0.37, faceId * 0.71)) - 0.5) * 0.12;

            float crackLine = 1.0 - smoothstep(0.018, 0.055, edgeDistance);
            float edgeLead = crackLine * 0.05;
            float revealCoord = propagation + cellOffset - edgeLead;

            float filledMask =
                smoothstep(revealCoord - 0.05, revealCoord + 0.05,
                           uBreakProgress);

            float crackFront =
                smoothstep(revealCoord - 0.10, revealCoord + 0.01,
                           uBreakProgress);
            float crackMask = crackLine * crackFront;

            float fractureMask = max(filledMask, crackMask * 0.92);
            fractureMask = max(fractureMask,
                               smoothstep(0.985, 1.0, uBreakProgress));
            fractureMask = clamp(fractureMask, 0.0, 1.0);

            color = mix(color, vec3(0.0), fractureMask);

            vec2 starCoord = breakUv * BREAK_STAR_GRID;
            vec2 starCell = floor(starCoord);
            vec2 starLocal = fract(starCoord) - vec2(0.5);
            vec2 starCenter =
                (breakHash2(starCell + faceSeed * 5.0 +
                            vec2(5.0 + faceId, 11.0 + faceId * 1.73)) - 0.5) * 0.55;
            float starPresence =
                step(0.0015,
                     hash21(starCell + faceSeed * 17.0 +
                            vec2(3.7 + faceId, 9.1 + faceId * 0.41)));
            float starShape =
                1.0 - smoothstep(0.10, 0.24, length(starLocal - starCenter));
            float starBlink =
                0.35 + 0.65 * (0.5 + 0.5 * sin(
                    uTime * 3.85 +
                    hash21(starCell + faceSeed * 3.0 +
                           vec2(7.3 + faceId, 2.1 + faceId * 0.23)) *
                        6.2831853));
            float starMask =
                starPresence * 0.0 * starShape * starBlink *
                smoothstep(0.55, 0.9, filledMask); // starPresence * 0.2 <-- controls max star brightness

            color = mix(color, vec3(1.0), clamp(starMask, 0.0, 1.0));
        }
    }

    float dist = length(vWorldPos - uCameraPos);
    float fog = 1.0 - exp(-dist * uFogDensity);
    color = mix(color, uFogColor.rgb, fog);
    color = applySelectionHighlight(color, vWorldPos, faceNormal);

    FragColor = vec4(color, uAlpha);
}
