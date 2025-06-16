#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "write_ply.hpp"

struct GaussianCPU {
    glm::vec4 pos;
    glm::vec4 scale;
    glm::vec4 normal;
    glm::vec4 quat;
};

// quick quaternion-from-basis (X,Y,Z ortho)
inline glm::quat basisToQuat(const glm::vec3& X,
                             const glm::vec3& Y,
                             const glm::vec3& Z)
{
    glm::mat3 m(X, Y, Z);
    float t = m[0][0]+m[1][1]+m[2][2];
    glm::quat q;
    if(t>0){
        float s = glm::sqrt(t+1.f)*2.f;
        q.w = .25f*s;
        q.x = (m[2][1]-m[1][2])/s;
        q.y = (m[0][2]-m[2][0])/s;
        q.z = (m[1][0]-m[0][1])/s;
    }else{            // fallback branch (rare in practice)
        q = glm::quat(1,0,0,0);
    }
    return q;
}

inline std::vector<GaussianCPU>
sampleTriangleCPU(const glm::vec3& p0,
                  const glm::vec3& p1,
                  const glm::vec3& p2,
                  int m /*grid resolution*/)
{
    std::vector<GaussianCPU> out;
    out.reserve((m+1)*(m+2)/2);

    glm::vec3 e1 = p1-p0;
    glm::vec3 e2 = p2-p0;
    glm::vec3 n  = glm::normalize(glm::cross(e1,e2));

    // orthonormal basis X,Y,Z (Z = normal)
    glm::vec3 X = glm::normalize(e1);
    glm::vec3 Y = glm::normalize(glm::cross(n, X));
    glm::quat Q = basisToQuat(X,Y,n);

    float su = glm::length(e1) / float(m);
    // perpendicular component of e2 w.r.t X for isotropy in tangent plane
    glm::vec3 e2_perp = e2 - glm::dot(e2,X)*X;
    float sv = glm::length(e2_perp) / float(m);
    glm::vec3 S(su, sv, 1e-7f);

    for(int u=0; u<=m; ++u)
    {
        for(int v=0; v<=m-u; ++v)
        {
            float fu = float(u)/m;
            float fv = float(v)/m;
            float fw = 1.0f - fu - fv;

            glm::vec3 P = fu*p1 + fv*p2 + fw*p0;

            GaussianCPU g;
            g.pos   = glm::vec4(P,1);
            g.scale = glm::vec4(S,0);
            g.normal= glm::vec4(n,0);
            g.quat  = glm::vec4(Q.x,Q.y,Q.z,Q.w);
            out.push_back(g);
        }
    }
    return out;
}
