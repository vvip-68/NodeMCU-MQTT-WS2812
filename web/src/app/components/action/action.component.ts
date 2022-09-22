import {Component, ElementRef, Input, OnDestroy, OnInit, ViewChild} from '@angular/core';
import {ActionModel, ActionType, StateModel} from "../../models/effect.model";
import {WebsocketService} from "../../services/websocket/websocket.service";
import {debounceTime, Subject, takeUntil} from "rxjs";
import {ManagementService} from "../../services/management/management.service";
import {RGBA} from "ngx-color";

@Component({
  selector: 'app-action',
  templateUrl: './action.component.html',
  styleUrls: ['./action.component.scss']
})
export class ActionComponent implements OnInit, OnDestroy {

  actionType = ActionType;

  @ViewChild('card', {static: true}) card!: ElementRef;
  @ViewChild('icon', {static: true}) icon!: ElementRef;
  @ViewChild('indicator', {static: true}) indicator!: ElementRef;

  @Input() disabled: boolean = false;
  @Input() tooltip: string = '';

  @Input()
  get config(): ActionModel {
    return this._config;
  }

  set config(cfg: ActionModel) {
    this._config = cfg;
    this.updateValues();
  }

  private _config!: ActionModel;

  get state(): StateModel {
    return this._state;
  }

  set state(state: StateModel) {
    this._state = state;
    if (this._config) {
      switch (this._config.key) {
        case 'PWR':
          this._config.value = state.power;
          this.updateValues();
          break;
        case 'BR':
          this._config.value = state.brightness;
          this.updateValues();
          break;
        case 'RGB':
          this._config.value = state.color;
          this.updateValues();
          break;
      }
    }
  }

  private _state = new StateModel();

  private destroy$ = new Subject();

  constructor(public socketService: WebsocketService,
              public effectService: ManagementService) {
  }

  ngOnInit() {
    this.effectService.state$
      .pipe(takeUntil(this.destroy$), debounceTime(100))
      .subscribe((state: StateModel) => {
        this.state = state;
      });
  }

  private updateValues() {
    if (this.config && this.config.mode !== ActionType.NONE) {

      this.card.nativeElement.classList.add(`card_${this.config.key.toLowerCase()}`);
      this.icon.nativeElement.classList.add(this.config.mode);
      if (this.config.submode && this.config.submode.length > 0) {
        this.icon.nativeElement.classList.add(this.config.submode);
      }

      let sval = '';
      if (this.config.mode === ActionType.POWER || this.config.mode === ActionType.BRIGHT) {
        if (typeof this.config.onoff === 'boolean') {
          sval = this._state.power && this.config.onoff ? 'ON' : 'OFF';
        } else if (typeof this.config.onoff === 'function') {
          // Если питание отключено - все другие "кнопки" также должны быть отключены
          sval = this._state.power ? this.config.onoff(this.config.value) : 'OFF';
        }
      } else if (this.config.mode === ActionType.COLOR && (this.config.submode === 'color_set' || this.config.submode === 'color_user')) {
        const color = <RGBA>this._config.value;
        if (color) {
          this.icon.nativeElement.style.backgroundColor = `rgba(${color.r},${color.g},${color.b},1)`;
        }
      }

      if (sval.length > 0) {
        this.icon.nativeElement.classList.remove('state_OFF', 'state_ON');
        this.icon.nativeElement.classList.add(`state_${sval}`);
      }

    }
  }

  doSetCommand() {
    if (this.config.set) {
      let command = '';
      if (this.config.key === 'PWR' || this.state.power) {
        if (typeof this.config.set === 'string' && this.config.set.length > 0) {
          command = this.config.set as string;
        } else if (typeof this.config.set === 'function') {
          command = this.config.set(this.config.value) ?? '';
        }
        if (command.length > 0) {
          this.socketService.sendText(command);
        }
      }
    }
  }

  ngOnDestroy() {
    this.destroy$.next(true);
    this.destroy$.complete();
  }
}
