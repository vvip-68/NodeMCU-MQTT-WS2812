import {Component, Inject, OnInit} from '@angular/core';
import {MAT_DIALOG_DATA, MatDialogRef} from "@angular/material/dialog";
import {EffectModel} from "../../models/effect.model";
import {WebsocketService} from "../../services/websocket/websocket.service";
import {ManagementService} from "../../services/management/management.service";
import {Subscription} from "rxjs";

@Component({
  selector: 'app-effect-params',
  templateUrl: './effect-params.component.html',
  styleUrls: ['./effect-params.component.scss']
})
export class EffectParamsComponent implements OnInit {

  public readonly tooltip1 =
    "Время воспроизведения эффекта.\n\n" +
    "Если включен \"Случайный выбор эффекта\" - время выбирается автоматически 15..60 секунд.\n\n" +
    "Если \"Случайный выбор эффекта\" отключен - эффект\nбудет воспроизводиться указанное время, затем сменится на другой.";

  public readonly tooltip2 =
    "Скорсть воспроизведения эффекта.\n\n" +
    "Чем выше значение, тем быстрее будет динамика эффекта в целом.";

  public readonly tooltip3 =
    "Количество сегментов.\n\nДлинную ленту можно разбить на несколько сегментов, " +
    "эффект будет повторяться синхронно в каждом сегменте.";

  public readonly tooltip4 =
    "Шаг изменения параметра эффекта.\n\n" +
    "Для составных эффектов управляет динамикой изменения " +
    "внутреннего эффекта\n\n" +
    "Например эффект - бегущие по ленте полоски (первый эффект), " +
    "каждая полоска изменяет цвет по мере продвижения.\n\n" +
    "Перемещение полоски определяется параметром \"Скорость\", " +
    "Изменения цвета полоски - параметром \"Шаг изменения\".";

  public model: EffectModel = new EffectModel();
  public isLoaded = false;
  private subscription: Subscription;

  get speed(): number {
    // Скорость в model.speed - 0 (не используется) или 1..128 если используется - это задержка в цикле loop() контроллера
    // То есть - чем больше значение model.speed - тем медленнее работает эффект
    // В слайдере нужно показывать обращенное значение, чтобы увеличение слайдера в большую строону уменьшало задержку (увеличивало скорость)
    return this.model.speed == 0 ? 0 : 129 - this.model.speed;
  }

  set speed(val: number) {
    if (this.model.speed > 0) {
      this.model.speed = 129 - val;
    }
  }

  constructor(public dialogRef: MatDialogRef<EffectParamsComponent>,
              public socketService: WebsocketService,
              public effectService: ManagementService,
              @Inject(MAT_DIALOG_DATA) private data: any) {
    this.model = data.model;

    this.subscription = this.effectService.editModel$
      .subscribe((model) => {
        if (model) {
          Object.assign(this.model, model);
          this.isLoaded = true;
        }
      });
  }

  ngOnInit(): void {
    this.dialogRef.disableClose = true;
  }

  cancel($event: MouseEvent) {
    this.subscription.unsubscribe();
    this.dialogRef.close();
  }

  apply($event: MouseEvent) {
    // Применить новые параметры
    // PM:N:T:D:S:P:U:A - установить для режима N указанные параметры
    //  N - номер режима - 2..MAX_EFFECT
    //  T - время "проигрывания" режима 15..255 сек
    //  D - задержка между циклами (т.е. фактически задаетскорость "проигрывания" режима
    //  S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
    //  P - значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
    //  U - 0 - не используется в автоматической смене режимов, 1 - режим используется
    //  A - 0 - просто изменить параметры режима, 1 - изменить параметры и активировать
    const command = `PM:${this.model.id}:${this.model.duration}:${this.model.speed}:${this.model.segments}:${this.model.step}:${this.model.fav ? 1 : 0}:1`;
    this.socketService.sendText(command);
    // Сохранить настройки в постоянную память
    this.socketService.sendText('SV');
    this.subscription.unsubscribe();
    this.dialogRef.close();
  }

  toggle() {
    this.model.fav = !this.model.fav;
  }
}
