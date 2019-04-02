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
    Clock,
    BufferGeometry,
    Geometry,
    Vector3,
    Face3
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

Math.degToRad = function(deg) {
    return deg * Math.PI / 180;
}

class Site {
    
    constructor() {
        this.isActive = false;
        this.clock = new Clock(false);

        this.scene = new Scene();
        this.camera = new PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 4000 );
        this.camera.position.set(0, 100, 200);

        const renderer = new WebGLRenderer();
        renderer.setPixelRatio(window.devicePixelRatio);
        renderer.setSize(window.innerWidth, window.innerHeight);
        document.body.appendChild(renderer.domElement);
        this.renderer = renderer;

        const controls = new OrbitControls(this.camera, this.renderer.domElement);
        controls.enableDamping = true;
        controls.dampingFactor = 0.25;
        controls.maxPolarAngle = 1.48353;
        controls.enableKeys = false;
        this.controls = controls;

        this._initGUI();

        this.animate = this.animate.bind(this);
        this._onWindowResize = this._onWindowResize.bind(this);

        window.addEventListener('resize', this._onWindowResize);
    }

    _updateView() {
        if(this.exteriorView) {
            this.controls.enabled = true;
            this.controls.target.set(0, 0, 0);
            this.camera.position.set(0, 100, 200);
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

    _updateCar(start) {
        // Ensure that start position cannot be changed when car is moving
        if(!this.isActive) {
            this.car.position.z = start;
        }
    }

    _updateBlindspot() {
        const lead = this.blindspotLeadingAngle;
        const trail = this.blindspotTrailingAngle;
        const carDistance = this.car.position.z;
        const angle_difference = 180 - this.angleOfIntersection;

        //Issues occur when leading angle or trailing angle are equal to 180 - AngleIntersection
        //At this point, the distance values become unpredictable and large
        const lAngle = Math.degToRad(lead);
        const tAngle = Math.degToRad(trail);
        const iAngle = Math.degToRad(this.angleOfIntersection);

        //CSED = Car Shadow Edge Distance
        const CSED_Numerator = (carDistance * Math.sin(iAngle));
        let CSED_Trail =  CSED_Numerator / Math.sin((Math.PI - (tAngle + iAngle))); //Distance from trailing edge of blindspot shadow to car
        const CSED_Lead = CSED_Numerator / Math.sin((Math.PI - (lAngle + iAngle))); //Distance from leading edge blindspot shadow to car

        if (angle_difference < trail)
        {
            CSED_Trail = 100000; //Fixed distance on trailing edge of shadow to prevent visual issues when there is no intersection between the road and the trailing edge
        }

        //Cleaning things up so its easier to read the next set of operations
        const Opp_Lead = Math.sin(lAngle);
        const Opp_Trail = Math.sin(tAngle);
        const Adj_Lead = -Math.cos(lAngle);
        const Adj_Trail = -Math.cos(tAngle);

        //Update blindspot vertices
        this.blindspot.geometry.vertices[0].set(0, .1, carDistance);
        this.blindspot.geometry.vertices[1].set(CSED_Trail * Opp_Trail, .1, (CSED_Trail * Adj_Trail) + carDistance);
        this.blindspot.geometry.vertices[2].set(CSED_Lead * Opp_Lead, .1, (CSED_Lead * Adj_Lead) + carDistance);
        this.blindspot.geometry.verticesNeedUpdate = true;
    }

    _replay() {
        this.car.position.z = this.carStartDistance;

        this.isActive = false;
        this.clock.stop();
        this.GUI.__controllers[2].name('Play');

        this.GUI.updateDisplay();
    }

    _reset() {
        this._setUIElements();
        this._updateView();
        this._updateFov(this.fov)
        this._updateRoad(this.angleOfIntersection);
        this._replay();
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
        uiCar.add(this, 'carStartDistance', 0, 1000).step(0.1).name('Car Starting Distance').onChange(this._updateCar.bind(this));
        uiCar.add(this, 'carSpeed', 0, 100).step(0.1).name('Car Speed');

        const uiBike = this.GUI.addFolder('Bike');
        uiBike.add(this, 'bikeStartDistance', 0, 1000).step(0.1).name('Bike Starting Distance');
        uiBike.add(this, 'bikeSpeed', 0, 100).step(0.1).name('Bike Speed');

        this.GUI.add(this, '_reset').name('Reset');
        this.GUI.add(this, '_replay').name('Replay');
        this.GUI.add(this, 'run').name('Play').onFinishChange((val) => {
            const controller = this.GUI.__controllers[2];
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

        //Blindspot
        const gBlindspot = new Geometry();
        gBlindspot.vertices.push(
            new Vector3(0, 0.1, 0), //Origin
            new Vector3(0, 0.1, 0), //Leading
            new Vector3(0, 0.1, 0), //Trailing
        );
        gBlindspot.faces.push(
            new Face3(0, 1, 2)
        );
        const mBlindspot = new MeshBasicMaterial({color : 0xff0000});
        const blindspot = new Mesh(gBlindspot, mBlindspot);
        this.blindspot = blindspot;
        this.scene.add(blindspot);

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
        if(this.isActive) {
            this.clock.start();
        } else {
            this.clock.stop();
        }
    }

    animate() {
        requestAnimationFrame(this.animate);

        const delta = this.clock.getDelta();

        if(this.car.position.z > 0) this.car.position.z -= delta * this.carSpeed;

        this._updateBlindspot()

        this.controls.update();
        this.renderer.render(this.scene, this.camera);
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
    site.animate();
});