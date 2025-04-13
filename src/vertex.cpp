#include "vertex.hpp"

namespace fhope {
    bool Vertex2D::operator==(const Vertex2D &o) const {
        return o.color == this->color && o.position == this->position && o.uv == this->uv;
    }

    bool Vertex3D::operator==(const Vertex3D &o) const {
        return o.color == this->color && o.position == this->position && o.uv == this->uv;
    }
}


namespace std {
    size_t hash<fhope::Vertex2D>::operator() (fhope::Vertex2D const &toHash) const {
        return (
            (
                hash<glm::vec2>()(toHash.position) ^
                (hash<glm::vec3>()(toHash.color) << 1)
            ) >> 1
        ) ^ (
            hash<glm::vec2>()(toHash.uv) << 1
        );
    }



    size_t hash<fhope::Vertex3D>::operator() (fhope::Vertex3D const &toHash) const {
        return (
            (
                hash<glm::vec3>()(toHash.position) ^
                (hash<glm::vec3>()(toHash.color) << 1)
            ) >> 1
        ) ^ (
            hash<glm::vec2>()(toHash.uv) << 1
        );
    }
}
