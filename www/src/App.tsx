import React, { Component } from 'react';
import './App.scss';
import { Alert, AppState, OSettings, Playlist, Station } from 'models';
import Player from 'Player';
import Settings from 'Settings';
import Stations from 'Stations';
import { ApiService } from 'api.service';

class App extends Component<any, AppState> {

  private newStationId: number = -1;

  private apiService: ApiService;

  constructor(props) {
    super(props);
    this.state = {
      visibleContent: "player",
      alerts: [],
      editStationId: this.newStationId,
      filteredStations: [],
      newStation: new Station(this.newStationId, "", ""),
      playlist: new Playlist(),
      settings: new OSettings(),
      showNewCard: false,
      stationFilter: "",
      stationValue: 0
    };
    this.apiService = new ApiService();
  }

  componentDidMount = () => {
    this.getPlaylist();
    this.getSettings();
  }


  getSettings = () => {
    this.apiService.getSettings().then((data: OSettings) => this.setSettings(data));
  };

  getPlaylist = () => {
    this.apiService.getPlaylist().then((data: Playlist) => this.setPlaylist(data));
  };

  changeContent = (target: string) => {
    this.setState({
      visibleContent: target
    });
  }

  setPlaylist = (data: Playlist) => {
    this.setState({
      playlist: data
    }, () => {
      this.setState({
        filteredStations: this.state.playlist.stations.filter(s => s.desc.toLowerCase().includes(this.state.stationFilter))
      });
    });
  }

  switchStation = (id: number) => {
    this.setState({
      stationValue: id
    });
    this.apiService.changeToStation(id);
  }

  setSettings = (data: OSettings) => {
    this.setState({
      settings: data
    });

  }

  clearAlerts = () => {
    this.setState({
      alerts: []
    });
  }

  addAlert = (alert: Alert) => {
    let alerts = this.state.alerts;
    alerts.push(alert);
    this.setState({
      alerts: alerts
    });
  }
  setFilter = (filter: string) => {
    this.setState({
      stationFilter: filter,
      filteredStations: this.state.playlist.stations.filter(s => s.desc.toLowerCase().includes(filter))
    })
  }

  showAddStation = () => {
    this.setState({
      showNewCard: true,
      newStation: new Station(this.newStationId, "", "")
    });
  }

  cancelEdit = () => {
    this.setState({
      showNewCard: false,
      editStationId: this.newStationId,
      newStation: new Station(this.newStationId, "", "")
    });
  }

  editStation = (id: number) => {
    let param = this.state.playlist;
    let station = param.stations.find(s => s.id === id);
    this.setState({
      editStationId: id,
      newStation: station
    });
  }

  reorderStations = () => {
    let param = this.state.playlist;
    let i = 0;
    param.stations.forEach(s => {
      s.id = i++;
    });
    this.setState({
      playlist: param
    });
  }

  addStation = () => {
    let playlist = this.state.playlist;
    let edited: number;
    if (this.state.newStation.id === this.newStationId) {
      playlist.stations.push(this.state.newStation);
    }
    else {
      edited = playlist.stations.findIndex(s => s.id === this.state.newStation.id);
      playlist.stations[edited] = this.state.newStation;
    }
    this.setState({
      playlist: playlist,
      filteredStations: this.state.playlist.stations.filter(s => s.desc.toLowerCase().includes(this.state.stationFilter.toLowerCase()))
    });
    this.reorderStations();
    this.cancelEdit();
  }

  updateNewDesc = (event, id: number) => {
    let playlist = this.state.playlist;
    let station: Station;
    if (id === this.newStationId) {
      station = this.state.newStation;
      station.desc = event.target.value;
      this.setState({
        newStation: station
      });
    }
    else {
      station = playlist.stations.find(s => s.id === id);
      station.desc = event.target.value;
      this.setState({
        playlist: playlist
      });
    }
  }

  updateNewUrl = (event, id: number) => {
    let playlist = this.state.playlist;
    let station: Station;
    if (id === this.newStationId) {
      station = this.state.newStation;
      station.url = event.target.value;
      this.setState({
        newStation: station
      });
    }
    else {
      station = playlist.stations.find(s => s.id === id);
      station.url = event.target.value;
      this.setState({
        playlist: playlist
      });
    }
  }

  deleteStation = (id: number) => {
    let param = this.state.playlist;
    param.stations = param.stations.filter(s => s.id !== id);
    if (id === param.defaultStation) {
      param.defaultStation = 0;
    }
    this.reorderStations();
    this.setState({
      playlist: param,
      filteredStations: this.state.playlist.stations.filter(s => s.desc.toLowerCase().includes(this.state.stationFilter.toLowerCase()))
    });
  }

  setStationAsDefault = (id: number) => {
    let param = this.state.playlist;
    param.defaultStation = id;
    this.setState({
      playlist: param
    });
  }

  showContent() {
    switch (this.state.visibleContent) {
      case 'player': {
        return <Player stationValue={this.state.stationValue} playlist={this.state.playlist} setPlaylist={(data) => this.setPlaylist(data)} switchStation={(id) => this.switchStation(id)}></Player>;
      }
      case 'settings': {
        return <Settings state={this.state} settings={this.state.settings} setSettings={(data) => this.setSettings(data)} getSettings={() => this.getSettings()} clearAlerts={() => this.clearAlerts()} addAlert={(data) => this.addAlert(data)}></Settings>;
      }
      case 'stations': {
        return <Stations state={this.state} setStationAsDefault={(id) => this.setStationAsDefault(id)} deleteStation={(id) => this.deleteStation(id)} editStation={(id) => this.editStation(id)} updateNewUrl={(event, id) => this.updateNewUrl(event, id)} cancelEdit={() => this.cancelEdit()} showAddStation={() => this.showAddStation()} updateNewDesc={(event, id) => this.updateNewDesc(event, id)} addStation={() => this.addStation()} setFilter={(filter) => this.setFilter(filter)} clearAlerts={() => this.clearAlerts()} addAlert={(data) => this.addAlert(data)} setPlaylist={(data) => this.setPlaylist(data)}></Stations>;
      }
    }
  }

  buttonClasses(isActive: boolean) {
    let classes = ["btn", "btn-outline-light", "btn-block"];
    if (isActive) {
      classes.push("active");
    }
    return classes.join(" ");
  }
  render = () => {
    return (
      <>
        <div className='sidebar'>
          <h1>OradiO</h1>
          <div className="menu">
            <button className={this.buttonClasses(this.state.visibleContent === 'player')} aria-current="page" onClick={() => this.changeContent('player')}>
              <i className="bi-play"></i>Lejátszó
            </button>
            <button className={this.buttonClasses(this.state.visibleContent === 'settings')} onClick={() => this.changeContent('settings')}>
              <i className="bi-gear"></i>Beállítások
            </button>
            <button className={this.buttonClasses(this.state.visibleContent === 'stations')} onClick={() => this.changeContent('stations')}>
              <i className="bi-broadcast-pin"></i>Állomások
            </button>
          </div>
        </div>
        <div id="content" className="content">
          {
            this.showContent()
          }

        </div>
      </>
    );
  }
}
export default App;
