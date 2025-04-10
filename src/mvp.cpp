#include "mvp.hpp"

namespace fhope {
    MVP::MVP() : model(1.0f), view(1.0f), projection(1.0f), dirty(false) {
        this->outMVP = projection * view * model;
    }



    MVP::MVP(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) :
            model(model), view(view), projection(projection), dirty(false) {
        this->outMVP = projection * view * model;
    }


    
    MVP::MVP(const MVP &o) : model(o.model), view(o.view), projection(o.projection), dirty(o.dirty), outMVP(o.outMVP) {}



    glm::mat4 MVP::get_model() const {
        return this->model;
    }



    glm::mat4 MVP::get_view() const {
        return this->view;
    }



    glm::mat4 MVP::get_projection() const {
        return this->projection;
    }



    void MVP::set_model(const glm::mat4 &newModel) {
        this->model = newModel;
        this->dirty = true;
    }



    void MVP::set_view(const glm::mat4 &newView) {
        this->view = newView;
        this->dirty = true;
    }



    void MVP::set_projection(const glm::mat4 &newProjection) {
        this->projection = newProjection;
        this->dirty = true;
    }



    bool MVP::is_dirty() const {
        return this->dirty;
    }



    glm::mat4 MVP::get_mvp() {
        if (this->dirty) {
            this->outMVP = this->projection * this->view * this->model;
            this->dirty = false;
        }

        return this->outMVP;
    }



    glm::mat4 MVP::recompute_mvp() {
        this->outMVP = this->projection * this->view * this->model;
        this->dirty = false;

        return this->outMVP;
    }


    
    glm::mat4 MVP::get_latest_mvp() {
        return this->outMVP;
    }
}
