#pragma once

#include <glm/glm.hpp>

namespace fhope {
    struct MVP {
        private:
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 projection;

            glm::mat4 outMVP;
            bool dirty;
        
        public:
            MVP();
            MVP(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection);
            MVP(const MVP &o);

            glm::mat4 get_model() const;
            glm::mat4 get_view() const;
            glm::mat4 get_projection() const;

            void set_model(const glm::mat4 &newModel);
            void set_view(const glm::mat4 &newView);
            void set_projection(const glm::mat4 &newProjection);

            bool is_dirty() const;

            glm::mat4 get_mvp();

            glm::mat4 recompute_mvp();

            glm::mat4 get_latest_mvp();
    };
}
