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
    Math
} from 'three';

import * as Dat from 'dat.gui';

import OrbitControls from 'three-orbitcontrols';

import sky_nx from '../assets/sky_nx.png'; import sky_ny from '../assets/sky_ny.png'; import sky_nz from '../assets/sky_nz.png';
import sky_px from '../assets/sky_px.png'; import sky_py from '../assets/sky_py.png'; import sky_pz from '../assets/sky_pz.png';

import grass from '../assets/bmGrass.jpg';
import asphalt from '../assets/bmAsphalt.jpg';

const sky = [
    sky_nz, sky_pz, 
    sky_py, sky_ny,
    sky_px, sky_nx
];

class Site {
    
    constructor() {
        this.isActive = false;

        this.scene = new Scene();
        this.camera = new PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 4000 );
        this.camera.position.set(0, 10, 20);

        const renderer = new WebGLRenderer();
        renderer.setPixelRatio(window.devicePixelRatio);
        renderer.setSize(window.innerWidth, window.innerHeight);
        renderer.shadowMap.enabled = true;
        renderer.shadowMapSoft = true;
        renderer.shadowCameraNear = 3;
        renderer.shadowCameraFar = this.camera.far;
        renderer.shadowCameraFov = 50;
        renderer.shadowMapBias = 0.0039;
        renderer.shadowMapDarkness = 0.5;
        renderer.shadowMapWidth = 1024;
        renderer.shadowMapHeight = 1024;
        document.body.appendChild(renderer.domElement);
        this.renderer = renderer;

        const controls = new OrbitControls(this.camera, this.renderer.domElement);
        controls.enableDamping = true;
        controls.dampingFactor = 0.25;
        controls.maxPolarAngle = 1.48353;
        controls.enableKeys = false;
        this.controls = controls;

        this._initGUI();

        this._animate = this._animate.bind(this);
        this._onWindowResize = this._onWindowResize.bind(this);

        window.addEventListener('resize', this._onWindowResize);
    }

    _updateView() {
        if(this.exteriorView) {
            this.controls.enabled = true;
            this.controls.target.set(0, 0, 0);
            this.camera.position.set(0, 10, 20);
        } else {
            this.controls.enabled = false;
        }
    }

    _updateFov(fov) {
        this.camera.fov = fov;
        this.camera.updateProjectionMatrix();
    }

    _updateRoad(angle) {
        this.road.rotation.set(
            this.road.rotation.x,
            Math.degToRad(angle),
            this.road.rotation.z
        );
    }

    _reset() {
        this._setUIElements();
        this._updateView();
        this._updateFov(this.fov)
        this._updateRoad(this.angleOfIntersection);
        this.GUI.updateDisplay();
    }

    _setUIElements() {
        this.exteriorView = true;

        this.fov = 75;
        this.camera.fov = this.fov;

        this.angleOfIntersection = 69;
        this.blindspotLeadingAngle = 19.4;
        this.blindspotTrailingAngle = 27.1;
        this.carStartDistance = 100;
        this.carSpeed = 18;
        this.bikeStartDistance = 39;
        this.bikeSpeed = 7;
    }

    _initGUI() {
        this._setUIElements();

        this.GUI = new Dat.GUI({width : 340, hideable : true});

        const uiCamera = this.GUI.addFolder('Camera');
        uiCamera.add(this, 'exteriorView').name('Exterior View').onFinishChange(this._updateView.bind(this));
        uiCamera.add(this, 'fov', 0.1, 179.9).step(0.1).onChange(this._updateFov.bind(this));

        const uiRoad = this.GUI.addFolder('Road');
        uiRoad.add(this, 'angleOfIntersection', 0, 180).step(0.1).name('Angle of Intersection').onChange(this._updateRoad.bind(this));

        const uiBlindspot = this.GUI.addFolder('Blindspot');
        uiBlindspot.add(this, 'blindspotLeadingAngle', 0, 45).step(0.1).name('Blindspot Leading Angle');
        uiBlindspot.add(this, 'blindspotTrailingAngle', 0, 45).step(0.1).name('Blindspot Trailing Angle');

        const uiCar = this.GUI.addFolder('Car');
        uiCar.add(this, 'carStartDistance', 0, 1000).step(0.1).name('Car Starting Distance');
        uiCar.add(this, 'carSpeed', 0, 100).step(0.1).name('Car Speed');

        const uiBike = this.GUI.addFolder('Bike');
        uiBike.add(this, 'bikeStartDistance', 0, 1000).step(0.1).name('Bike Starting Distance');
        uiBike.add(this, 'bikeSpeed', 0, 100).step(0.1).name('Bike Speed');

        this.GUI.add(this, '_reset').name('Reset Scene');
        this.GUI.add(this, 'run').name('Pause').onFinishChange((val) => {
            const controller = this.GUI.__controllers[1];
            if(this.isActive) controller.name('Pause');
            else controller.name('Play');
        });

        this.GUI.updateDisplay();
    }

    _initScene() {
        const dLight = new DirectionalLight( 0xd7cbb1, 0.4 );
        dLight.position.x = 500;
        dLight.position.y = 500;
        dLight.position.z = 500;
        dLight.castShadow = true;
        this.scene.add( dLight );

        const aLight = new AmbientLight(0xaabbff, 0.2);
        this.scene.add(aLight);

        this._initGeometry();
    }



    _initGeometry() {
        //Textures
        const tGrass = new TextureLoader().load(grass);
        tGrass.wrapS = RepeatWrapping;
        tGrass.wrapT = RepeatWrapping;
        tGrass.repeat.set(100, 100);

        const tRoad = new TextureLoader().load(asphalt);
        tRoad.wrapS = RepeatWrapping;
        tRoad.wrapT = RepeatWrapping;
        tRoad.repeat.set(0.5, 250);

        //Meshes
        const gSkybox = new BoxGeometry(3000, 3000, 3000);
        const mSkybox = [];
        for(let i = 0; i < 6; i++) {
            mSkybox.push(new MeshBasicMaterial({
                map : new TextureLoader().load(sky[i]),
                side : DoubleSide
            }));
        }
        const skybox = new Mesh(gSkybox, mSkybox);
        this.scene.add(skybox);

        const gGround = new PlaneGeometry(2000, 2000);
        const mGround = new MeshPhongMaterial({
            color : 0x567d46,
            specular    : 0x567d46,
			shininess   : 10,
            bumpMap     : tGrass,
            bumpScale   : 0.1
        });
        const ground = new Mesh(gGround, mGround);
        ground.receiveShadow = true;
        ground.rotateX(-1.5708);
        this.scene.add(ground);

        const gRoad = new BoxGeometry(4, 0.1, 2000);
        const mRoad = new MeshPhongMaterial({
            color       : 0xa3a3a3,
            specular    : 0xa3a3a3,
            shininess   : 2,
            bumpMap     : tRoad,
            bumpScale   : 0.1
        });
        const cRoad = new Mesh(gRoad, mRoad);
        cRoad.receiveShadow = true;
        cRoad.name = 'cRoad';
        this.scene.add(cRoad);

        const bRoad = cRoad.clone();
        bRoad.name = 'bRoad';
        bRoad.rotation.set(0, Math.degToRad(this.angleOfIntersection), 0);
        this.road = bRoad;
        this.scene.add(bRoad);

        const gCar = new BoxGeometry(1.5, 2.0, 4.0);
        const mCar = new MeshPhongMaterial({color : 0xffffff});
        const car = new Mesh(gCar, mCar);
        car.castShadow = true;
        car.position.set(0.0, 1.05, this.carStartDistance);
        this.car = car;
        this.scene.add(car);

    }

    build() {
        const promise = new Promise((resolve, reject) => {
            this._initScene();
            resolve();
        });
        return promise;
    }

    run() {
        this.isActive = !this.isActive;
        if(this.isActive) this._animate();
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