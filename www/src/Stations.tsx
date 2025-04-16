import React, { Component } from 'react';
import './Stations.scss';
import { Alert, Playlist, Station, StationsProps } from 'models';
import { ApiService } from 'api.service';
import AlertComponent from 'AlertComponent';

class Stations extends Component<any, any> {

  private newStationId: number = -1;
  private apiService: ApiService;
  private importInput = React.createRef<HTMLInputElement>();

  constructor(public props: StationsProps) {
    super(props);

    this.apiService = new ApiService();
  }

  fileChanged = (event) => {
    let file = event.target.files[0];
    file.text().then(content => {
      let playlist = new Playlist();
      Object.assign(playlist, JSON.parse(content));
      if (playlist.defaultStation < 0 || playlist.stations.length < 1) {
        this.props.addAlert({ type: 'warning', content: "Playlist nem megfelelő", timeout: 3, dismiss: this.props.clearAlerts });
      }
      else {
        this.props.setPlaylist(playlist);
      }
      this.importInput.current.value = "";
    })

  }

  buttonClasses(colorClass: string = "") {
    let classes = ["btn"];
    if (colorClass) {
      classes.push("btn-outline-" + colorClass);
    }
    else {
      classes.push("btn-outline-light");
    }
    return classes.join(" ");
  }
  cardHeaderClass(id: number) {
    let classes = ["card-header"];
    if (id == this.props.state.playlist.defaultStation) {
      classes.push("bg-secondary");
    }
    return classes.join(" ");
  }

  save = () => {
    let param = this.props.state.playlist;
    this.apiService.savePlaylist(param).then(() => {
      this.props.addAlert({ type: 'info', content: "Mentés kész", timeout: 3, dismiss: this.props.clearAlerts });
    })
  }

  exportStations = () => {
    const b = new Blob([JSON.stringify(this.props.state.playlist)]);
    const fileURL = window.URL.createObjectURL(b);
    const link = document.createElement("a");
    link.href = fileURL;
    link.download = "oradio-playlist.json";
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }

  render = () => {
    return (
      <>
        <h3>Állomások</h3>
        <div className="cardContainer">
          <div className="card menu">
            <div className='card-header'>
              Műveletek
            </div>
            <div className='card-body'>
              <div className="normal-state">
                <button title='Változtatások mentése' className='btn btn-outline-primary' onClick={() => { this.save() }} ><i className="bi-floppy"></i></button>
                <button title='Új állomás' className='btn btn-outline-light' disabled={this.props.state.showNewCard || this.props.state.editStationId !== this.newStationId} onClick={() => { this.props.showAddStation() }} ><i className="bi-plus-square"></i></button>
                <button title='Importálás' className='btn btn-outline-light' disabled={this.props.state.showNewCard || this.props.state.editStationId !== this.newStationId} onClick={() => { this.importInput.current.click() }} ><i className="bi-upload"></i></button>
                <input onChange={(event) => { this.fileChanged(event) }} ref={this.importInput} type="file" accept="application/json" hidden id="input" multiple />
                <button title='Exportálás' className='btn btn-outline-light' disabled={this.props.state.showNewCard || this.props.state.editStationId !== this.newStationId} onClick={() => { this.exportStations() }} ><i className="bi-download"></i></button>
              </div>
              {this.props.state.alerts.map(alert => {
                return <AlertComponent key={alert.type} timeout={alert.timeout} content={alert.content} type={alert.type} dismiss={alert.dismiss}></AlertComponent>
              })}
            </div>
          </div>
          <div className="card menu">
            <div className='card-header'>
              Keresés
            </div>
            <div className='card-body'>
              <span>Keresés</span>
              <input type="text" onChange={(c) => this.props.setFilter(c.target.value)} value={this.props.state.stationFilter}></input>
            </div>
          </div>
          <div className='card' hidden={!this.props.state.showNewCard}>
            <div className="card-header">
              <h5 className="card-title" title='Állomás neve'><input type="text" value={this.props.state.newStation.desc} onChange={(event) => this.props.updateNewDesc(event, this.newStationId)}></input></h5>
            </div>
            <div className='card-body'>
              <h6 className="card-subtitle mb-2 text-muted" title='Állomás url'><input type="text" value={this.props.state.newStation.url} onChange={(event) => this.props.updateNewUrl(event, this.newStationId)}></input></h6>
              <div className="edit-state">
                <button title='Mentés' className={this.buttonClasses()} onClick={() => this.props.addStation()}><i className="bi-floppy"></i></button>
                <button title='Mégsem' className={this.buttonClasses("danger")} onClick={() => this.props.cancelEdit()}><i className="bi-trash"></i></button>
              </div>
            </div>
          </div>
          {this.props.state.filteredStations.map(pl =>
            <div className='card station' key={pl.id}>
              <div className={this.cardHeaderClass(pl.id)}>
                <h5 className="card-title" hidden={this.props.state.editStationId === pl.id}>{pl.desc}</h5>
                <h5 className="card-title" hidden={this.props.state.editStationId !== pl.id}><input type="text" value={this.props.state.newStation.desc} onChange={(event) => this.props.updateNewDesc(event, pl.id)}></input></h5>
              </div>
              <div className='card-body'>
                <h6 className="card-subtitle mb-2 text-muted" hidden={this.props.state.editStationId === pl.id}>{pl.url}</h6>
                <h6 className="card-subtitle mb-2 text-muted" hidden={this.props.state.editStationId !== pl.id}><input type="text" value={this.props.state.newStation.url} onChange={(event) => this.props.updateNewUrl(event, pl.id)}></input></h6>
                <div className="buttons">
                  <div className="normal-state" hidden={this.props.state.editStationId === pl.id}>
                    <button title='Szerkesztés' className={this.buttonClasses()} disabled={this.props.state.showNewCard} onClick={() => this.props.editStation(pl.id)}><i className="bi-pencil"></i></button>
                    <button title='Törlés' className={this.buttonClasses("danger")} onClick={() => this.props.deleteStation(pl.id)}><i className="bi-trash"></i></button>
                    <button title='Alapértelmezett' className={this.buttonClasses()} hidden={pl.id === this.props.state.playlist.defaultStation} onClick={() => this.props.setStationAsDefault(pl.id)}><i className="bi-house-heart"></i></button>
                  </div>
                  <div className="edit-state" hidden={this.props.state.editStationId !== pl.id}>
                    <button title='Mentés' className={this.buttonClasses()} onClick={() => this.props.addStation()}><i className="bi-floppy"></i></button>
                    <button title='Mégsem' className={this.buttonClasses("danger")} onClick={() => this.props.cancelEdit()}><i className="bi-x-square"></i></button>
                  </div>
                </div>
              </div>
            </div>
          )
          }
        </div>
      </>
    );
  }
}
export default Stations;
