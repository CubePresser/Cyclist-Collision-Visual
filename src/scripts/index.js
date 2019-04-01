import {
    Scene,
    PerspectiveCamera,
    WebGLRenderer,
    Mesh,
    BoxGeometry,
    MeshBasicMaterial,
    TextureLoader,
    DoubleSide,
    DirectionalLight,
    MeshPhongMaterial,
    AmbientLight,
    PlaneGeometry,
    RepeatWrapping,
} from 'three';

import OrbitControls from 'three-orbitcontrols';

import sky_nx from '../assets/sky_nx.png'; import sky_ny from '../assets/sky_ny.png'; import sky_nz from '../assets/sky_nz.png';
import sky_px from '../assets/sky_px.png'; import sky_py from '../assets/sky_py.png'; import sky_pz from '../assets/sky_pz.png';

import grass from '../assets/grass-bump.jpg';

const sky = [
    sky_nz, sky_pz, 
    sky_py, sky_ny,
    sky_px, sky_nx
];

class Site {
    
    constructor() {
        this.isActive = true;

        this.scene = new Scene();
        this.camera = new PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 4000 );
        this.camera.position.set(0, 10, 20);

        const renderer = new WebGLRenderer();
        renderer.setPixelRatio(window.devicePixelRatio);
        renderer.setSize(window.innerWidth, window.innerHeight);
        document.body.appendChild(renderer.domElement);
        this.renderer = renderer;

        const controls = new OrbitControls(this.camera);
        controls.enableDamping = true;
        controls.dampingFactor = 0.25;
        this.controls = controls;

        this._animate = this._animate.bind(this);
        this._onWindowResize = this._onWindowResize.bind(this);

        window.addEventListener('resize', this._onWindowResize);
    }

    _initScene() {
        const dLight = new DirectionalLight( 0xd7cbb1, 0.4 );
        dLight.position.x = 500;
        dLight.position.y = 500;
        dLight.position.z = 500;
        this.scene.add( dLight );

        const aLight = new AmbientLight(0xaabbff, 0.2);
        this.scene.add(aLight);

        this._initGeometry();
    }



    _initGeometry() {
        const gSkybox = new BoxGeometry(1000, 1000, 1000);
        const mSkybox = [];
        for(let i = 0; i < 6; i++) {
            mSkybox.push(new MeshBasicMaterial({
                map : new TextureLoader().load(sky[i]),
                side : DoubleSide
            }));
        }
        const skybox = new Mesh(gSkybox, mSkybox);
        this.scene.add(skybox);

        const tGrass = new TextureLoader().load(grass);
        tGrass.wrapS = RepeatWrapping;
        tGrass.wrapT = RepeatWrapping;
        tGrass.repeat.set(25, 25);
        const gGround = new PlaneGeometry(1000, 1000);
        const mGround = new MeshPhongMaterial({
            color : 0x567d46,
            specular    : 0x567d46,
			shininess   : 10,
            bumpMap     : tGrass,
            bumpScale   : 0.1
        });

        const ground = new Mesh(gGround, mGround);
        ground.rotateX(-1.5708);
        this.scene.add(ground);
    }

    build() {
        const promise = new Promise((resolve, reject) => {
            this._initScene();
            resolve();
        });
        return promise;
    }

    run() {
        this.isActive = true;
        this._animate();
    }

    pause() {
        this.isActive = false;
    }

    _animate(timestamp) {
        if(this.isActive) {
            requestAnimationFrame(this._animate);
            this.controls.update();
            this.renderer.render(this.scene, this.camera);
        }
    }

    _onWindowResize() {
        const width = window.innerWidth;
        const height = window.innerHeight;
        this.renderer.setSize(width, height);
        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();
    }
};

const site = new Site();
site.build().then(() => {
    site.run();
});