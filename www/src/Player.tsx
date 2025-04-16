import React, { ChangeEvent, Component } from 'react';
import './Player.scss';
import { PlayerProps } from 'models';
import { ApiService } from 'api.service';

class Player extends Component<any, any> {

  private apiService;
  //private _props: PlayerProps;

  constructor(public props: PlayerProps) {
    super(props);

    this.apiService = new ApiService();
    //this._props = props;
  }

  changeToStation = (stationId: number) => {
    this.props.switchStation(stationId);
  }

  changeStation = (element: ChangeEvent<HTMLSelectElement>) => {
    this.changeToStation(Number(element.target.value));
  };

  setVolume(up: boolean) {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', 'api/setvolume', true);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.onreadystatechange = () => {
      // Call a function when the state changes.
      if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
        // Request finished. Do processing here.
      }
    };
    xhr.send(JSON.stringify(up));
  }

  jumpToStation = (advance: boolean) => {
    if (advance) {
      if (this.props.stationValue + 1 < this.props.playlist.stations.length) {
        this.props.switchStation(this.props.stationValue + 1);
      }
    }
    else {
      if (this.props.stationValue - 1 >= 0) {
        this.props.switchStation(this.props.stationValue - 1);
      }
    }
  }
  buttonClasses(isDisabled: boolean) {
    let classes = ["btn", "btn-outline-light", "btn-block"];
    if (isDisabled) {
      classes.push("disabled");
    }
    return classes.join(" ");
  }

  render = () => {
    return (
      <>
        <h3>Lejátszó</h3>
        <div className="cardContainer">
          <div className='card menu'>
            <div className="card-header">
              Állomás
            </div>
            <div className='card-body'>
              <select aria-label="Station" onChange={this.changeStation} value={this.props.stationValue}>
                {this.props.playlist.stations.map(pl =>
                  <option key={pl.id} value={pl.id}>{pl.desc}</option>
                )
                }
              </select>
              <div className="normal-state">
                <button title="Legelső" className={this.buttonClasses(this.props.stationValue === 0)} onClick={() => this.changeToStation(0)}><i className='bi-skip-start'></i></button>
                <button title="Előző" className={this.buttonClasses(this.props.stationValue === 0)} onClick={() => this.jumpToStation(false)}><i className='bi-skip-backward'></i></button>
                <button title="Következő" className={this.buttonClasses(this.props.stationValue === this.props.playlist.stations.length - 1)} onClick={() => this.jumpToStation(true)}><i className='bi-skip-forward'></i></button>
                <button title="Utolsó" className={this.buttonClasses(this.props.stationValue === this.props.playlist.stations.length - 1)} onClick={() => this.changeToStation(this.props.playlist.stations.length - 1)}><i className='bi-skip-end'></i></button>
                <button title="Halkít" className={this.buttonClasses(false)} onClick={() => this.setVolume(false)}><i className='bi-volume-down'></i></button>
                <button title="Hangosít" className={this.buttonClasses(false)} onClick={() => this.setVolume(true)}><i className='bi-volume-up'></i></button>
              </div>
            </div>
          </div>
        </div>
      </>
    );
  }
}
export default Player;
